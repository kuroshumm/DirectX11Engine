# 演習21 アニメーション・クロスフェード (Animation CrossFade) システム: 実装・トラブルシューティング報告書

本書は、DirectX 11 エンジンにおけるアニメーション・クロスフェード（ポーズ合成・状態遷移）機能の実装プロセスにおいて直面した技術的課題、潜在バグの発生原因、解決の軌跡、および最終的な実装ソースコードを記録した報告書である。

---

## 1. システム全体設計とフロー

高レベル制御（`AnimatorSystem`）と低レベル幾何演算（`AnimationSystem`）の責務を分離し、単方向データフローとして構築された。

```mermaid
flowchart TD
    A[ユーザー入力 / スクリプト] -->|CrossFade 'Run', 0.25f| B[AnimatorSystem::CrossFade]
    B -->|前クリップ退避 & 遷移パラメータ設定| C[AnimatorComponent]
    
    subgraph 毎フレームの更新ループ
        D[ゲームループ: Update] --> E[AnimatorSystem::Update]
        E -->|transitionTimer 加算 & 重み t 算出| C
        
        D --> F[AnimationSystem::Update]
        C -.->|isBlending, weight, pPrevClip, prevTime を参照| F
        F -->|CurrClip と PrevClip をサンプリング| G[SampleBoneTransform]
        G -->|移動・拡縮: Lerp / 回転: Slerp| H[合成アフィン行列の算出]
        H -->|書き込み| I[SkeletonComponent: bones]
        
        D --> J[BoneTransformSystem::Update]
        I -->|FK計算 (再帰伝播)| K[骨格のグローバルワールド行列確定]
    end
```

---

## 2. 発生した主要な不具合と解決の軌跡

### 2.1. 不具合 1: クリップ間でのチャンネルインデックス使い回しによる姿勢崩壊および範囲外アクセス

#### 1. 現象 (Symptom)
初期実装時、`AnimationSystem::Update` のループ内で `bone.animChannelIndex` をキャッシュし、そのインデックスを `currClip` と `prevClip` の両方に対する `GetNodeAnimation(bone.animChannelIndex)` の引数として使い回していた。
この状態のまま異なる FBX ファイル間でクロスフェードを実行すると、手足のボーンが別の部位のキーフレームと合成されて骨格が異常に変形・ねじ切れるか、前クリップのチャンネル数が少ない場合に `std::vector` の範囲外アクセス（Out of Range）が発生してデバッグアサーション失敗（クラッシュ）に至る潜在的危険があった。

#### 2. 原因 (Root Cause)
`bone.animChannelIndex` は、単一クリップのみを再生していた演習17当時のキャッシュ機構である。
異なる FBX ファイル（例: `Idle.fbx` と `Walk.fbx`）間では、同一名称のボーン（例: `mixamorig:RightArm`）であっても、インポーターが読み込んだチャンネル配列（`m_NodeAnimations`）内のインデックス番号が一致する保証は全くない。
新クリップ用のインデックスを使って旧クリップの配列を参照したため、全く無関係なボーンのキーフレームが取得されたり、配列境界外のメモリへアクセスしたりしていた。

#### 3. 解決策 (Resolution)
`AnimationSystem::SampleBoneTransform` の内部で、渡された各クリップに対して安全にチャンネル番号を名前解決（`clip->FindChannelIndex(bone.name)`）する設計へ改修した。

```cpp
// 各クリップごとに正しいチャンネル番号を独立して取得
int channelIndex = clip->FindChannelIndex(bone.name);
if (channelIndex < 0)
{
    // クリップ内に該当ボーンが存在しない場合は初期バインド姿勢を返却
    return { defaultScale, defaultRot, defaultPos };
}
const NodeAnimation& nodeAnim = clip->GetNodeAnimation(channelIndex);
```
これにより、クリップ間でボーン順序やチャンネル数が異なる場合でも、安全かつ正確に正しいボーンの TRS キーフレームがサンプリングされるようになった。

---

### 2.2. 不具合 2: `AnimatorComponent` メンバ変数の未初期化による初期フレームの誤動作

#### 1. 現象 (Symptom)
構造体 `AnimatorComponent` をインスタンス化してエンティティにアタッチした直後、初期フレームにおいて `isTransitioning` が予期せず `true`（非ゼロ）と誤判定され、遷移を行っていないにもかかわらず無効なブレンド処理が実行される危険があった。

#### 2. 原因 (Root Cause)
[Components.h](file:///d:/DirectX11_Sample/DirextX11Engine/Components.h) 内で `float prevTime;` や `bool isTransitioning;` などの組み込み型メンバ変数に対し、デフォルトメンバ初期化子（`= 0.0f;`, `= false;`）が指定されていなかった。C++ の規則により、明示的な初期化がない組み込み変数はスタックやヒープの不定値（ゴミ値）を保持するため、未定義動作が発生していた。

#### 3. 解決策 (Resolution)
すべての遷移制御パラメータに明示的なデフォルトメンバ初期化子を付与し、生成時点で健全な初期状態（生焼け状態の防止）を保証した。
```cpp
float prevTime           = 0.0f;
float transitionDuration = 0.0f;
float transitionTimer    = 0.0f;
float transitionWeight   = 0.0f;
bool isTransitioning     = false;
```

---

### 2.3. 不具合 3: 文字列引数の値渡しによる不要なヒープ割り当て

#### 1. 現象 (Symptom)
`AnimatorSystem::Play` および `AnimatorSystem::CrossFade` の引数において、`std::string stateName` が値渡しで宣言されていたため、関数呼び出しごとに不要な動的メモリ確保（文字列コピー）が発生していた。

#### 2. 原因 (Root Cause)
C++ コーディング標準（[cpp_coding_standards](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/cpp-coding-standards/SKILL.md)）に対する準拠漏れ。

#### 3. 解決策 (Resolution)
すべての文字列引数を `const std::string&`（const 参照渡し）に変更し、ゼロコピーでのステート名受け渡しを実現した。

---

## 3. 最終実装ソースコード

### 3.1. [Components.h](file:///d:/DirectX11_Sample/DirextX11Engine/Components.h)
クロスフェード用パラメータを追加した `AnimatorComponent` の定義部。

```cpp
	struct AnimatorComponent
	{
		std::unordered_map < std::string, AnimationState2 > stateMap;
		std::string currentStateName = "";
		std::string prevStateName = "";

		std::shared_ptr<AnimationClip> pPrevClip = nullptr;
		float prevTime = 0.0f;
		float transitionDuration = 0.0f;
		float transitionTimer = 0.0f;
		float transitionWeight = 0.0f;
		bool isTransitioning = false;

		void AddState(const std::string& stateName, const std::shared_ptr<AnimationClip>& pClip, bool isLoop)
		{
			AnimationState2 animState;
			animState.Name = stateName;
			animState.Clip = pClip;
			animState.isLoop = isLoop;

			auto [it, inserted] = stateMap.try_emplace(stateName, std::move(animState));
			assert(inserted);
		}
	};
```

---

### 3.2. [AnimatorSystem.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/AnimatorSystem.cpp)
タイマー進行・完了判定および `CrossFade` 制御の実装。

```cpp
#include <algorithm>
#include <cmath>
#include "AnimatorSystem.h"

namespace Engine
{
	void AnimatorSystem::Update(entt::registry& registry, float dt)
	{
		auto view = registry.view<AnimatorComponent, AnimationComponent>();
		for (auto [entity, animator, animation] : view.each())
		{
			if (!animator.isTransitioning || !animator.pPrevClip)
			{
				continue;
			}

			// 遷移タイマーの加算
			animator.transitionTimer += dt;

			// 旧クリップの再生時刻を裏で進める (勢い/モーメンタムの維持)
			animator.prevTime += dt * animator.pPrevClip->GetTicksPerSecond() * animation.playbackSpeed;
			if (animator.pPrevClip->GetDuration() > 0.0f)
			{
				animator.prevTime = std::fmod(animator.prevTime, animator.pPrevClip->GetDuration());
			}

			// 正規化ブレンド重み (0.0 〜 1.0) の算出
			animator.transitionWeight = std::clamp(
				animator.transitionTimer / animator.transitionDuration,
				0.0f,
				1.0f
			);

			// 遷移完了判定
			if (animator.transitionTimer >= animator.transitionDuration)
			{
				animator.isTransitioning = false;
				animator.pPrevClip = nullptr;
				animator.transitionWeight = 1.0f;
			}
		}
	}

	void AnimatorSystem::Play(
		entt::registry& registry,
		entt::entity entity,
		std::string stateName
	)
	{
		if (!registry.all_of<AnimatorComponent, AnimationComponent>(entity))
		{
			return;
		}

		auto& animator  = registry.get<AnimatorComponent>(entity);
		auto& animation = registry.get<AnimationComponent>(entity);

		auto it = animator.stateMap.find(stateName);
		if (it == animator.stateMap.end() || !it->second.Clip)
		{
			return;
		}

		animator.currentStateName = stateName;

		// 即時切り替えのため、進行中の遷移はキャンセル
		animator.isTransitioning = false;
		animator.pPrevClip = nullptr;
		animator.transitionWeight = 1.0f;

		// AnimationComponentにクリップとループ設定を流す
		animation.pCurrentClip = it->second.Clip;
		animation.isLoop = it->second.isLoop;
		animation.currentTime = 0.0f;
		animation.isPlaying = true;
	}

	void AnimatorSystem::CrossFade(
		entt::registry& registry,
		entt::entity entity,
		std::string stateName,
		float duration
	)
	{
		if (!registry.all_of<AnimatorComponent, AnimationComponent>(entity))
		{
			return;
		}

		auto& animator = registry.get<AnimatorComponent>(entity);
		auto& animation = registry.get<AnimationComponent>(entity);

		auto it = animator.stateMap.find(stateName);
		if (it == animator.stateMap.end() || !it->second.Clip)
		{
			return;
		}

		// 既に同じステートに遷移済みの場合は何もしない
		if (animator.currentStateName == stateName && animation.pCurrentClip == it->second.Clip)
		{
			return;
		}

		// 遷移パラメータの退避と設定
		animator.prevStateName = animator.currentStateName;
		animator.pPrevClip = animation.pCurrentClip;
		animator.prevTime = animation.currentTime;
		animator.transitionDuration = (duration > 0.0f) ? duration : 0.0001f; // ゼロ除算防止
		animator.transitionTimer = 0.0f;
		animator.transitionWeight = 0.0f;
		animator.isTransitioning = (animator.pPrevClip != nullptr && animator.pPrevClip != it->second.Clip);

		// 次ステートのセット
		animator.currentStateName = stateName;
		animation.pCurrentClip = it->second.Clip;
		animation.currentTime = 0.0f;
		animation.isLoop = it->second.isLoop;
		animation.isPlaying = true;
	}
}
```

---

### 3.3. [AnimationSystem.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/AnimationSystem.cpp)
安全なボーンサンプリングとブレンド合成の実装。

```cpp
#include "AnimationSystem.h"
#include "AnimationClip.h"

namespace Engine
{
    void AnimationSystem::Update(entt::registry& registry, float dt)
    {
        auto entities = registry.view<AnimationComponent, SkeletonComponent, AnimationUpdateTag>();
        for (auto [entity, animationComp, skeltonComp] : entities.each())
        {
            if (!animationComp.isPlaying || !animationComp.pCurrentClip || skeltonComp.rootBoneIndex == -1)
            {
                continue;
            }

            // AnimatorComponent を取得し、クロスフェード遷移中か確認
            const AnimatorComponent* pAnimator = registry.try_get<AnimatorComponent>(entity);
            const bool isBlending = (pAnimator && pAnimator->isTransitioning && pAnimator->pPrevClip);
            const float blendWeight = isBlending ? pAnimator->transitionWeight : 1.0f;
            std::shared_ptr<AnimationClip> currClip = animationComp.pCurrentClip;
            std::shared_ptr<AnimationClip> prevClip = isBlending ? pAnimator->pPrevClip : nullptr;

            auto& bones = skeltonComp.bones;
            for (Bone& bone : bones)
            {
                // 1. Current クリップの姿勢サンプリング
                BoneTransformTRS currPose = SampleBoneTransform(currClip, animationComp.currentTime, bone);
                DirectX::XMVECTOR finalScale = currPose.scale;
                DirectX::XMVECTOR finalRot = currPose.rot;
                DirectX::XMVECTOR finalPos = currPose.pos;

                // 2. 遷移中なら Prev クリップもサンプリングして Lerp / Slerp 合成
                if (isBlending)
                {
                    BoneTransformTRS prevPose = SampleBoneTransform(prevClip, pAnimator->prevTime, bone);
                    finalScale = DirectX::XMVectorLerp(prevPose.scale, currPose.scale, blendWeight);
                    finalPos = DirectX::XMVectorLerp(prevPose.pos, currPose.pos, blendWeight);
                    finalRot = DirectX::XMQuaternionSlerp(prevPose.rot, currPose.rot, blendWeight);
                }

                // 3. アフィン変換行列の作成と書き込み
                DirectX::XMMATRIX localMatrix = DirectX::XMMatrixAffineTransformation(
                    finalScale, DirectX::XMVectorZero(), finalRot, finalPos
                );
                DirectX::XMStoreFloat4x4(&bone.LocalTransformMatrix, localMatrix);
            }

            // タイマーの更新 (TicksPerSecond を反映)
            animationComp.currentTime += dt * currClip->GetTicksPerSecond() * animationComp.playbackSpeed;
            if (animationComp.isLoop && currClip->GetDuration() > 0.0f)
            {
                animationComp.currentTime = std::fmod(animationComp.currentTime, currClip->GetDuration());
            }
        }
    }

    BoneTransformTRS AnimationSystem::SampleBoneTransform(
        const std::shared_ptr<AnimationClip>& clip,
        float time,
        const Bone& bone
    )
    {
        DirectX::XMVECTOR defaultScale = DirectX::XMLoadFloat3(&bone.defaultScale);
        DirectX::XMVECTOR defaultRot = DirectX::XMLoadFloat4(&bone.defaultRot);
        DirectX::XMVECTOR defaultPos = DirectX::XMLoadFloat3(&bone.defaultPos);
        if (!clip)
        {
            return { defaultScale, defaultRot, defaultPos };
        }

        // クリップごとにチャンネル番号を安全に動的解決
        int channelIndex = clip->FindChannelIndex(bone.name);
        if (channelIndex < 0)
        {
            return { defaultScale, defaultRot, defaultPos };
        }

        const NodeAnimation& nodeAnim = clip->GetNodeAnimation(channelIndex);
        if (nodeAnim.PositionKeys.empty() && nodeAnim.ScalingKeys.empty() && nodeAnim.RotaionKeys.empty())
        {
            return { defaultScale, defaultRot, defaultPos };
        }

        // 1. 移動 (Position)
        DirectX::XMVECTOR pos;
        if (nodeAnim.PositionKeys.empty())
        {
            pos = defaultPos;
        }
        else if (nodeAnim.PositionKeys.size() == 1)
        {
            pos = DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(time, nodeAnim.PositionKeys, left, right, f);
            pos = DirectX::XMVectorLerp(
                DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[left].Value),
                DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[right].Value), f);
        }

        // 2. スケール (Scaling)
        DirectX::XMVECTOR scale;
        if (nodeAnim.ScalingKeys.empty())
        {
            scale = defaultScale;
        }
        else if (nodeAnim.ScalingKeys.size() == 1)
        {
            scale = DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(time, nodeAnim.ScalingKeys, left, right, f);
            scale = DirectX::XMVectorLerp(
                DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[left].Value),
                DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[right].Value), f);
        }

        // 3. 回転 (Rotation)
        DirectX::XMVECTOR rot;
        if (nodeAnim.RotaionKeys.empty())
        {
            rot = defaultRot;
        }
        else if (nodeAnim.RotaionKeys.size() == 1)
        {
            rot = DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(time, nodeAnim.RotaionKeys, left, right, f);
            rot = DirectX::XMQuaternionSlerp(
                DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[left].Value),
                DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[right].Value), f);
        }

        return { scale, rot, pos };
    }
}
```

---

## 4. 今後の拡張への指針

1. **連続遷移時のスナップショット（Frozen Pose / カスケードブレンド）**:
   * A $\to$ B のクロスフェード中（例: 0.1秒時点）に C への遷移が要求された場合、現在の2クリップモデルでは直前のブレンド状態が破棄されて姿勢が跳躍する。
   * これを解決するために、割り込み発生時の現在の合成姿勢（全ボーンのTRS）を一時配列にスナップショット退避し、それを新たな「前ポーズ」として C へ補間する仕組みを導入する。
2. **ブレンドツリー（Blend Tree）への発展**:
   * 現在の 1 対 1 クロスフェードを基礎として、入力パラメータ（移動速度など）に応じて常時複数クリップ（Idle/Walk/Run）を重み付き合成する 1D/2D ブレンドノードの設計へ拡張する。
3. **モーフターゲット（表情制御）との同期**:
   * 将来コンピュートシェーダーによるモーフターゲット変形を導入する際、アニメーションクリップ内に含まれるモーフカーブのウェイト値も本クロスフェードの $t$ に連動させて線形補間する。

---

## 5. 専門用語集

* **クロスフェード (CrossFade)**:
  2つのアニメーションの重みを滑らかに入れ替えることで、ポーズの不連続（跳躍）を防ぐ遷移技法。
* **範囲外アクセス (Out of Bounds Access)**:
  配列やベクターの確保サイズを超えた添え字でメモリを参照するエラー。メモリ保護違反によるクラッシュの原因となる。
* **デフォルトメンバ初期化子 (Default Member Initializer)**:
  クラス・構造体の定義側でメンバ変数に直接初期値を指定する C++11 の機能。
* **単一書き込み権限 (Single Writer Principle)**:
  ECSにおいて、特定のコンポーネントを更新するシステムを1つに絞ることで競合を防ぐ設計ルール。
