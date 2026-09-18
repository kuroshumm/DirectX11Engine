# 演習20 カメラ & カメラコントローラ ECS 化システム: 実装・トラブルシューティング報告書

本書は、DirectX 11 エンジンにおけるカメラおよびカメラコントローラの ECS（Entity Component System: EnTT）化の実装プロセスにおいて直面した技術的課題、バグの発生原因、解決の軌跡、および最終的な実装ソースコードを記録した報告書である。

---

## 1. システム全体設計とフロー

カメラおよびカメラコントローラを ECS 化することにより、入力からレンダリングに至るまでのパイプラインが以下の単方向データフローとして整理された。

```mermaid
flowchart TD
    A[ユーザー入力: Keyboard / Mouse] --> B[InputSystem]
    B --> C[CameraControllerSystem::Update]
    C -->|オイラー角蓄積・クランプ & 四元数回転変換| D[TransformComponent: position, rotation]
    D --> E[TransformSystem::Update]
    E -->|S * R * T 行列合成| F[TransformComponent: worldMatrix]
    F --> G[CameraSystem::Update]
    G -->|M_World^-1 逆行列計算| H[CameraComponent: viewMatrix, projectionMatrix, frustum]
    H --> I[RenderSystem::Render]
    I -->|ActiveCameraTag を持つ Entity を検索| J[視錐台カリング & GPU 定数バッファ転送]
    J --> K[モデル描画 (Draw)]
```

---

## 2. 発生した主要な不具合と解決の軌跡

### 2.1. 不具合 1: EnTT マルチコンポーネント View における MSVC テンプレート比較演算子エラー (C2678)

#### 1. 現象 (Symptom)
`RenderSystem.cpp` 内でアクティブカメラの存在判定を行うために `auto it = cameraView.begin(); if (it == cameraView.end())` と記述したところ、以下のコンパイルエラーが発生してビルドに失敗した。

```text
d:\DirectX11_Sample\DirextX11Engine\RenderSystem.cpp(12,17): error C2678: 二項演算子 '==': 型 'entt::internal::view_iterator<Type,false,3,0>' の左オペランドを扱う演算子が見つかりません (または変換できません)
        with [ Type=entt::basic_sparse_set<entt::entity,std::allocator<entt::entity>> ]
d:\DirectX11_Sample\DirextX11Engine\Library\entt\entt.hpp(3314,40): 'bool entt::internal::view_iterator<...>::operator ==(...) const noexcept' の可能性があります
```

#### 2. 原因 (Root Cause)
EnTT の複数コンポーネント走査用イテレータ `view_iterator` は、テンプレート引数に型だけでなく非型テンプレート引数（NTTP: `bool Checked, size_t Get, size_t Exclude`）を持つ。
EnTT 内で `template<typename Other, auto... Args> operator==(const view_iterator<Other, Args...> &other)` として比較演算子が定義されていたが、MSVC（Visual C++ コンパイラ）が非型テンプレート引数パック `auto... Args` のオーバーロード解決および型推論に失敗し、適合する `==` 演算子を発見できなかった。

#### 3. 解決策 (Resolution)
イテレータの直接比較（`it == end()`）を廃止し、EnTT の標準的な走査構文である `.each()` を用いてアクティブカメラを抽出する設計へリファクタリングした。`.each()` は内部でイテレータの `operator==` を経由せず SparseSet を直接走査するため、コンパイラ依存のテンプレート推論問題を完全に回避できる。

```cpp
// 修正後: .each() を用いた安全なアクティブカメラ取得
auto cameraView = registry.view<TransformComponent, CameraComponent, ActiveCameraTag>();
const CameraComponent* pActiveCamera = nullptr;
for (auto [entity, transform, camera] : cameraView.each())
{
    pActiveCamera = &camera;
    break; // 最初のアクティブカメラを採用
}
if (!pActiveCamera) return;
```

---

### 2.2. 不具合 2: 視錐台変数の重複宣言によるコンパイルエラー (C2086 / C2374)

#### 1. 現象 (Symptom)
`RenderSystem.cpp` のコンパイル時に、変数の多重定義エラーが発生した。

```text
RenderSystem.cpp(22,24): error C2374: 'frustum': 再定義されています。2 回以上初期化されています。
RenderSystem.cpp(22,24): error C2086: 'const Frustum &frustum': 再定義されました。
```

#### 2. 原因 (Root Cause)
コードの統合時に、同一スコープ内で `const Frustum& frustum = camera.frustum;` 行が重複して 2 行連続で記述されていた。

#### 3. 解決策 (Resolution)
重複した宣言行を削除し、単一の宣言に整理した。

---

### 2.3. 不具合 3: カメラエンティティ未登録およびシステム未生成によるモデル全非描画

#### 1. 現象 (Symptom)
コンパイルは正常に通るものの、アプリケーション実行時に画面上に 3D モデル（SwordMan 等）が 1 つも描画されず、背景クリア色のみが表示される状態となった。

#### 2. 原因 (Root Cause)
1. **アクティブカメラエンティティの未生成**: `App::InitGeometry` 内でモデル用エンティティは 50〜100 体生成されていたが、カメラ用のエンティティが生成されておらず、`TransformComponent`, `CameraComponent`, `FreeLookCameraControllerComponent`, `ActiveCameraTag` が Registry に登録されていなかった。その結果、`RenderSystem::Render` 内で `pActiveCamera` が `nullptr` となり、`if (!pActiveCamera) return;` のガード条件によって毎フレーム描画ループがスキップされていた。
2. **システム未生成**: `App::Initialize` で `m_cameraControllerSystem = std::make_unique<CameraControllerSystem>();` の生成が抜けていた。

#### 3. 解決策 (Resolution)
1. [`App.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/App/App.cpp) の `InitGeometry` 内でカメラエンティティを明示的に生成し、初期位置・初期ピッチ角クォータニオン・FOV・コントローラ設定・`ActiveCameraTag` を登録した。
2. `App::Initialize` で `m_cameraControllerSystem` を `std::make_unique` でインスタンス化した。
3. `App::Update` から不要となった古い `m_pSceneCameraOperator->Update(dt);` を削除した。

---

## 3. 最終実装ソースコード

### 3.1. [Components.h](file:///d:/DirectX11_Sample/DirextX11Engine/Components.h)
```cpp
#pragma once
#include <DirectXMath.h>
#include "Frustum.h"

namespace Engine
{
    // カメラの投影・ビュー情報および Frustum を保持するコンポーネント
    struct CameraComponent
    {
        float fov         = DirectX::XM_PIDIV4; // 45度
        float aspectRatio = 800.0f / 600.0f;
        float nearZ       = 0.1f;
        float farZ        = 1000.0f;

        DirectX::XMFLOAT4X4 viewMatrix;
        DirectX::XMFLOAT4X4 projectionMatrix;
        Frustum frustum;

        bool isProjectionDirty = true;
    };

    // 自由視点カメラ操作用パラメータコンポーネント
    struct FreeLookCameraControllerComponent
    {
        float moveSpeed   = 50.0f;
        float sensitivity = 0.3f;

        float pitch = 0.0f;
        float yaw   = 0.0f;

        float minPitch = -DirectX::XM_PIDIV2 + 0.01f;
        float maxPitch =  DirectX::XM_PIDIV2 - 0.01f;
    };

    // アクティブカメラ識別タグ
    struct ActiveCameraTag {};
}
```

### 3.2. [CameraControllerSystem.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/CameraControllerSystem.cpp)
```cpp
#include "CameraControllerSystem.h"
#include "Components.h"
#include "InputSystem.h"
#include "Vector2.h"
#include "Vector3.h"
#include <algorithm>

namespace Engine
{
    void CameraControllerSystem::Update(entt::registry& registry, float dt)
    {
        InputSystem& input = InputSystem::GetInstance();
        auto view = registry.view<TransformComponent, FreeLookCameraControllerComponent>();

        for (auto [entity, transform, controller] : view.each())
        {
            // 1. 回転処理 (マウス中ボタンドラッグ)
            if (input.IsKeyPressed(VK_MBUTTON))
            {
                Vector2 mouseDelta;
                if (input.TryGetMouseDelta(mouseDelta))
                {
                    controller.yaw   += mouseDelta.x * controller.sensitivity * dt;
                    controller.pitch += mouseDelta.y * controller.sensitivity * dt;

                    // ピッチ角の制限
                    controller.pitch = std::clamp(controller.pitch, controller.minPitch, controller.maxPitch);

                    // オイラー角からクォータニオンを生成して Transform に格納
                    DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(controller.pitch, controller.yaw, 0.0f);
                    DirectX::XMStoreFloat4(&transform.rotation, qRot);
                    transform.isDirty = true;
                }
            }

            // 2. 移動処理 (WASD 入力)
            DirectX::XMFLOAT3 moveDirLocal{ 0.0f, 0.0f, 0.0f };
            if (input.IsKeyPressed('W')) moveDirLocal.z += 1.0f;
            if (input.IsKeyPressed('S')) moveDirLocal.z -= 1.0f;
            if (input.IsKeyPressed('A')) moveDirLocal.x -= 1.0f;
            if (input.IsKeyPressed('D')) moveDirLocal.x += 1.0f;

            float lengthSq = moveDirLocal.x * moveDirLocal.x + moveDirLocal.y * moveDirLocal.y + moveDirLocal.z * moveDirLocal.z;
            if (lengthSq > 0.0001f)
            {
                DirectX::XMVECTOR vLocalMove = DirectX::XMLoadFloat3(&moveDirLocal);
                DirectX::XMVECTOR vRot = DirectX::XMLoadFloat4(&transform.rotation);
                DirectX::XMVECTOR vWorldMove = DirectX::XMVector3Rotate(DirectX::XMVector3Normalize(vLocalMove), vRot);

                DirectX::XMFLOAT3 worldDir;
                DirectX::XMStoreFloat3(&worldDir, vWorldMove);

                transform.position.x += worldDir.x * controller.moveSpeed * dt;
                transform.position.y += worldDir.y * controller.moveSpeed * dt;
                transform.position.z += worldDir.z * controller.moveSpeed * dt;
                transform.isDirty = true;
            }
        }
    }
}
```

### 3.3. [CameraSystem.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/CameraSystem.cpp)
```cpp
#include "CameraSystem.h"
#include "Components.h"

namespace Engine
{
    void CameraSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<TransformComponent, CameraComponent>();

        for (auto [entity, transform, camera] : view.each())
        {
            // 1. プロジェクション行列の更新
            if (camera.isProjectionDirty)
            {
                DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
                    camera.fov,
                    camera.aspectRatio,
                    camera.nearZ,
                    camera.farZ
                );
                DirectX::XMStoreFloat4x4(&camera.projectionMatrix, proj);
                camera.isProjectionDirty = false;
            }

            // 2. ビュー行列の更新 (M_View = M_World^-1)
            DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
            DirectX::XMMATRIX viewMat = DirectX::XMMatrixInverse(nullptr, worldMat);
            DirectX::XMStoreFloat4x4(&camera.viewMatrix, viewMat);

            // 3. 視錐台 (Frustum) の更新
            DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&camera.projectionMatrix);
            camera.frustum.Update(viewMat, projMat);
        }
    }
}
```

### 3.4. [RenderSystem.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/RenderSystem.cpp)
```cpp
#include "RenderSystem.h"
#include "Components.h"

namespace Engine
{
    void RenderSystem::Render(entt::registry& registry, Graphics& pGfx)
    {
        // 1. アクティブカメラの取得 (.each() による走査)
        auto cameraView = registry.view<TransformComponent, CameraComponent, ActiveCameraTag>();

        const CameraComponent* pActiveCamera = nullptr;
        for (auto [entity, transform, camera] : cameraView.each())
        {
            pActiveCamera = &camera;
            break;
        }

        if (!pActiveCamera)
        {
            return;
        }

        DirectX::XMMATRIX viewMat = DirectX::XMLoadFloat4x4(&pActiveCamera->viewMatrix);
        DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&pActiveCamera->projectionMatrix);
        const Frustum& frustum = pActiveCamera->frustum;

        // 2. モデルの走査と描画
        auto entities = registry.view<TransformComponent, ModelComponent>();
        for (auto [entity, transform, model] : entities.each())
        {
            DirectX::XMFLOAT3 center = transform.position;
            float localRadius = model.pModel->GetLocalBoundingSphere().Radius;
            float maxScale = (std::max)({ transform.scale.x, transform.scale.y, transform.scale.z });
            float worldRadius = localRadius * maxScale;

            if (!frustum.CheckSphere(center, worldRadius))
            {
                continue;
            }

            DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
            pGfx.Update(worldMat, viewMat, projMat);
            model.pModel->Draw();
        }
    }
}
```

---

## 4. 今後の拡張への指針

1. **複数カメラ・マルチビューポートへの対応**
   - `ActiveCameraTag` だけでなく、`ViewportComponent`（画面内描画矩形領域 $x, y, w, h$）や `Priority` を持たせることで、画面分割プレイ（Split Screen）やミニマップ用サブカメラの描画パスを容易に追加可能。
2. **多様なコントローラコンポーネントの追加**
   - **`OrbitCameraControllerComponent`**: 特定のターゲットエンティティの周囲を球面座標（方位角・仰角・距離）で旋回するカメラ。
   - **`ThirdPersonFollowComponent`**: プレイヤー後方をスプリング補間（Damping）および障害物レイキャスト衝突検知を伴って追従する三人称カメラ。
3. **カメライベントとポストプロセスの統合**
   - カメラシェイク（`CameraShakeComponent`）や FOV トランジション（ズームイン/アウト）を独立したシステムとして分離し、`TransformComponent` や `CameraComponent` に非破壊でオフセットを加算する設計が可能。

---

## 5. 専門用語集 (Glossary)

1. **非型テンプレート引数 (Non-Type Template Parameters: NTTP)**
   C++ テンプレートにおいて型（`typename T`）ではなく整数定数やポインタ等の値を渡す機能。EnTT の View では内部最適化のために多用されている。
2. **構造化束縛 (Structured Binding)**
   C++17 で導入された構文。`for (auto [entity, transform, camera] : view.each())` のように、タプルや集積体の要素を変数へ直接アンパックして受け取る記法。
3. **先行評価 (Eager Evaluation)**
   値が必要とされる前の定位置パイプラインで一括計算を完了させておくアーキテクチャ。遅延評価（Lazy Evaluation）に比べてデータフローが明示的になり、副作用や予測不能なジッターを防止できる。
4. **早期リターン (Early Return)**
   関数の事前条件が満たされない場合（アクティブカメラ未存在等）に、後続の重い処理を実行せず即座に関数を脱出する制御イディオム。