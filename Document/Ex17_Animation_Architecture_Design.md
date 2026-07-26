# 【第17回演習】骨格アニメーション・システム 設計図 ＆ 処理フロー解説書

本ドキュメントは、DirectX 11ゲームエンジンにおける「第17回演習：骨格アニメーションの再生と順運動学（FK）」で実装された主要機能のアーキテクチャ設計、処理フロー、およびクラス間の連携関係をまとめた設計仕様書です。

---

## 1. システム・アーキテクチャ（クラス設計）

アニメーションシステムは、主に3つの機能クラスと、それらを保持する `GameObject` / `App` で構成されています。

```mermaid
classDiagram
    class App {
        - std::shared_ptr~SkinnedModel~ m_pSkinnedModel
        - std::shared_ptr~AnimationClip~ m_pAnimClip
        - std::unique_ptr~Animator~ m_pAnimator
        + Initialize() HRESULT
        + Update(float deltaTime) void
    }

    class AnimationClip {
        - float m_Duration
        - float m_TicsPerSecond
        - std::unordered_map~string, NodeAnimation~ m_NodeAnimations
        + LoadFromFile(string filePath) HRESULT
        + GetNodeAnimation(string nodeName) const NodeAnimation&
        + GetDuration() float
        + GetTicksPerSecond() float
    }

    class Animator {
        - std::shared_ptr~SkinnedModel~ m_pModel
        - float m_currentTime
        + Update(float deltaTime) void
        - UpdateTimer(float deltaTime, float duration) void
        - CalcInterpolationFactor(vector~T~ keys, size_t& left, size_t& right, float& f) void
    }

    class SkinnedModel {
        - std::vector~Bone~ m_bones
        - Microsoft::WRL::ComPtr~ID3D11Buffer~ m_pPaletteBuffer
        + Update(float deltaTime) void
        + GetBones() std::vector~Bone~&
        - UpdateBoneTransform(int boneIndex, XMMATRIX parent) void
    }

    class Bone {
        + int parentIndex
        + std::vector~int~ childIndices
        + string name
        + XMFLOAT4X4 InverseBindMatrix
        + XMFLOAT4X4 LocalTransformMatrix
        + XMFLOAT4X4 GlobalTransformMatrix
        + XMFLOAT4X4 DefaultLocalMatrix
    }

    App *-- SkinnedModel
    App *-- AnimationClip
    App *-- Animator
    Animator o-- SkinnedModel : 参照・LocalTransformMatrixを更新
    SkinnedModel *-- Bone : 骨格構造の保持
```

---

## 2. 毎フレームの処理フロー（シーケンス図）

毎フレーム、ゲームループの更新処理（Update）が走った際、データがどのように各クラスを巡回し、GPUへと転送されるかを示すフロー図です。

```mermaid
sequenceDiagram
    autonumber
    actor App as ゲームループ (App::Update)
    participant Animator as 再生制御 (Animator::Update)
    participant Clip as アセット (AnimationClip)
    participant Model as モデル (SkinnedModel::Update)
    participant GPU as 描画デバイス (GPU VRAM)

    App->>Animator: 1. Update(deltaTime)
    Note over Animator: タイマー進行 (std::fmod)<br/>現在再生時刻を確定

    loop 全てのボーンに対して巡回
        Animator->>Clip: 2. GetNodeAnimation(boneName)
        Clip-->>Animator: アニメーションキー (S, R, T) を返却
        
        Note over Animator: 3. キーフレーム補間 (Lerp / Slerp)<br/>キーがない成分は DefaultLocalMatrix から抽出
        Note over Animator: 4. アフィン変換行列を合成 (S * R * T)<br/>※回転中心はローカル原点
        Animator->>Model: 5. bone.LocalTransformMatrix へストア (上書き書き換え)
    end

    App->>Model: 6. Update(deltaTime)
    
    rect rgb(200, 220, 255)
        Note over Model: 7. 順運動学 (FK) の計算<br/>UpdateBoneTransform(root, Identity)<br/>global = local * parentGlobal を再帰伝搬
    end

    loop 全てのボーンに対して巡回
        Note over Model: 8. 最終スキニング行列の算出<br/>skinningMat = InverseBindMatrix * GlobalTransformMatrix
        Note over Model: 9. HLSL用に行列を転置 (Transpose)
    end
    
    Model->>GPU: 10. 定数バッファ (PaletteBuffer) へマップして書き込み転送
    App->>GPU: 11. DrawCall 発行 (頂点シェーダーでスキニング変形を実行)
```

---

## 3. アセットロード時のデータフロー

モデル（FBX）とアニメーション（FBX）がそれぞれ別ファイルからロードされ、インポーターによってメモリ上にどのように展開・マージされるかを示す図です。

```mermaid
graph TD
    subgraph モデルロード (Model/SwordMan.fbx)
        A[Assimp::Importer] -->|ConvertToLeftHanded| B[各ノードの Transformation をロード]
        B -->|ConvertMatrix で転置コピー| C[Bone.LocalTransformMatrix]
        C -->|初期姿勢として不変退避| D[Bone.DefaultLocalMatrix]
        A -->|mOffsetMatrix をロード| E[Bone.InverseBindMatrix]
    end

    subgraph アニメーションロード (Animation/Sword And Shield Idle.fbx)
        F[Assimp::Importer] -->|ConvertToLeftHanded| G[アニメーションチャンネル群]
        G -->|_$AssimpFbx$_ サフィックスをトリミング| H[本来のボーン名へクレンジング]
        H -->|位置と回転のキーフレームを合流| I[NodeAnimation にマージして格納]
    end
```

---

## 4. 骨格構造（Bone）と行列パレットの対応設計

### 4.1. C++側：ボーン情報と定数バッファのメモリレイアウト
* 各ボーンは `m_bones` 配列（`std::vector<Bone>`）としてフラットに管理されています。
* GPUに送る行列パレット（`PaletteBufferData`）は、最大256個の 4x4 行列を保持する連続したメモリ領域です。

```
[C++ メモリ領域]
PaletteBufferData (アライメント16)
├─ Bones[0] : 転置された SkinningMatrix (Bone[0]用)
├─ Bones[1] : 転置された SkinningMatrix (Bone[1]用)
│  ...
└─ Bones[255]: 転置された SkinningMatrix (Bone[255]用)
```

### 4.2. HLSL（頂点シェーダー）側：スキニング計算
頂点シェーダー（`LambertianVertexShader.hlsl`）内では、頂点データ（`SkinnedVertex`）に含まれる **`BlendIndices`（影響するボーンのインデックス4個）** と **`BlendWeight`（各ボーンの影響重み4個）** を用いて、頂点位置をブレンド変形させます。

$$\mathbf{V}_{\text{final}} = \sum_{i=0}^{3} \left( \mathbf{V}_{\text{local}} \cdot \mathbf{M}_{\text{Bones[BlendIndices}[i]\text{]}} \right) \times \text{BlendWeight}[i]$$

* **HLSLコード設計**:
```hlsl
cbuffer PaletteBuffer : register(b1)
{
    matrix gBones[256]; // 行列パレット (定数バッファ)
};

struct VS_INPUT
{
    float3 Pos          : POSITION;
    float3 Normal       : NORMAL;
    float4 Color        : COLOR;
    float2 TexCoord     : TEXCOORD;
    uint4  BlendIndices : BLENDINDICES; // 影響するボーンのID 4つ
    float4 BlendWeight  : BLENDWEIGHT;  // 影響度（合計 1.0f）4つ
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // スキニング行列をブレンド合成
    matrix skinMatrix = 
        gBones[input.BlendIndices.x] * input.BlendWeight.x +
        gBones[input.BlendIndices.y] * input.BlendWeight.y +
        gBones[input.BlendIndices.z] * input.BlendWeight.z +
        gBones[input.BlendIndices.w] * input.BlendWeight.w;
        
    // 頂点位置をアニメーション後の絶対空間へ変形
    float4 localPos = float4(input.Pos, 1.0f);
    float4 skinnedPos = mul(localPos, skinMatrix);
    
    // ... 以降、通常の行列変換 (World * View * Projection) を実行 ...
}
```

---

## 5. 専門用語集

* **順運動学 (Forward Kinematics: FK)**
  * 関節のローカルの回転や移動から、親→子の階層順に乗算を繰り返すことで、各パーツの絶対的なワールド空間座標を算出する手法。
* **逆バインド行列 (Inverse Bind Matrix)**
  * メッシュの初期状態（バインドポーズ）におけるワールド空間内の頂点を、ボーンの関節のローカル原点まで引き戻すための行列。
* **行列パレット (Matrix Palette / スキニング行列配列)**
  * シェーダーへ一度に転送される、各ボーンの「InverseBindMatrix × GlobalTransformMatrix」を計算した変形用行列の一連の配列バッファ。
* **アフィン変換 (Affine Transformation)**
  * 拡大縮小（Scale）、回転（Rotation）、平行移動（Translation）の組み合わせで表現される、直線性と平行性を保つ幾何学的変換。
* **コロン・メジャー (Column-Major / 列優先) と ロー・メジャー (Row-Major / 行優先)**
  * 2次元の行列データをメモリ上の1次元配列にどのように並べるかという規格。DirectXMathは行優先（Row-Major）ですが、HLSLのデフォルトやシェーダーレジスタは列優先（Column-Major）を期待するため、CPUから送る前に転置（Transpose）処理が必要になります。
