# 演習19 階層トランスフォーム (Hierarchy Transform) システム: トラブルシューティング報告書

本ドキュメントは、演習19（階層トランスフォームシステム）の実装プロセスにおいて直面した技術的課題、バグの根本原因分析、およびその解決策を記録した実装報告書です。

---

## 1. システム全体設計とフロー

階層トランスフォームシステムは、ECSアーキテクチャに基づき、座標変更 $\rightarrow$ Dirty伝播 $\rightarrow$ 階層ワールド行列計算 $\rightarrow$ レンダリング/骨格系へのデータ引き渡しというパイプラインで動作します。

```mermaid
graph TD
    Start([フレーム開始]) --> Move[MovementSystem: 座標更新 & isDirty=true]
    Move --> HierSys[HierarchyTransformSystem: Update]
    
    subgraph Hierarchy Update Pipeline [階層更新パイプライン]
        HierSys --> FindRoot{ルートノードか?<br/>parent == null}
        FindRoot -- No --> Skip[スキップ]
        FindRoot -- Yes --> CheckRootDirty{isDirty == true?}
        CheckRootDirty -- Yes --> CompRootLocal[ルートローカル行列を計算]
        CheckRootDirty -- No --> LoadRootWorld[既存ワールド行列を再利用]
        
        CompRootLocal --> Recurse[UpdateSubtree: 子ノードへ再帰]
        LoadRootWorld --> Recurse
        
        subgraph Subtree Recursion [サブツリー再帰とDirty伝播]
            Recurse --> CheckChildNeed{childIsDirty || parentIsDirty}
            CheckChildNeed -- Yes --> CompChildWorld[子ローカル行列 * 親ワールド行列]
            CheckChildNeed -- No --> LoadChildWorld[既存ワールド行列を再利用]
            CompChildWorld --> NextChild[次の兄弟および子へ伝播]
            LoadChildWorld --> NextChild
        end
    end
    
    NextChild --> TransSys["TransformSystem (親子なしエンティティ限定)"]
    TransSys --> RenderSys[RenderSystem / BoneTransformSystem]
    RenderSys --> End([フレーム終了])
```

---

## 2. 発生した主要な不具合と解決の軌跡

### 2.1. 全走査再帰による計算量爆発 ($O(N^2)$) とメモリ断片化

*   **現象 (Symptom)**:
    エンティティ数が増加するにつれてフレームレートが著しく低下する潜在的リスクが存在した。また、可変長配列（`std::vector`）を用いた実装ではヒープアロケーションが多発していた。
*   **原因 (Root Cause)**:
    初期設計において、再帰関数の内部で `registry.view<TransformComponent, HierarchyComponent>()` を毎回全件ループし、`hierarchy.parent == parentEntity` に一致する子を探索していたため、計算量が $O(N^2) \sim O(N^D)$（$D$ は階層の深さ）に膨れ上がっていた。
*   **解決策 (Resolution)**:
    データ構造を **LCRS（Left-Child Right-Sibling）双方向リンクリスト**（`parent`, `firstChild`, `nextSibling`, `prevSibling`）に刷新。親は最初の子のみを把握し、子同士が兄弟リンクを辿ることで、探索ループを完全に排除し **$O(N)$ のツリー巡回** を達成。コンポーネントも固定サイズとなり動的メモリ確保をゼロにした。

---

### 2.2. 孫ノード以降へのDirty伝播の消失バグ

*   **現象 (Symptom)**:
    ルート（親）を移動させた際、直下の子（Child）は正常に追従移動するが、**孫（Grandchild）以降のエンティティがワールド空間に置き去りにされ移動しない**。
*   **原因 (Root Cause)**:
    `HierarchyTransformSystem.cpp` の `UpdateSubtree` において、子ノードへの再帰呼び出し時に `needUpdate` ではなく `childTransform.isDirty` を引数として渡していた。
    親が動いた（`parentIsDirty == true`）が、子自身はローカルで静止している（`childTransform.isDirty == false`）場合、孫に対して `parentIsDirty = false` が誤って伝播し、孫の更新がスキップされていた。
*   **解決策 (Resolution)**:
    子ノードへの再帰引数を、親のDirtyと自身のDirtyを結合した **`needUpdate`** に修正。親の動きが階層ツリー全体（末端の葉ノードまで）へ確実にカスケード伝播するようにした。

```cpp
// 修正前（バグ: 孫へ親のDirtyが伝わらない）
UpdateSubtree(registry, childHierarchy.firstChild, childWorldMat, childTransform.isDirty);

// 修正後（正常: needUpdate を伝播）
UpdateSubtree(registry, childHierarchy.firstChild, childWorldMat, needUpdate);
```

---

### 2.3. `TransformSystem` による階層計算結果の上書き破壊

*   **現象 (Symptom)**:
    `HierarchyTransformSystem` で親のワールド行列を乗算して算出した結果が反映されず、親子関係を設定しても子オブジェクトが原点基準のローカル位置に描画されてしまう。
*   **原因 (Root Cause)**:
    `App::Update` 内で `HierarchyTransformSystem` の直後に実行されていた `TransformSystem` が、`registry.view<TransformComponent>()` で全エンティティの `worldMatrix` を自身のローカル $S \times R \times T$ で無条件に上書きしていたため。
*   **解決策 (Resolution)**:
    `TransformSystem` のクエリに **`entt::exclude<HierarchyComponent>`** を指定。親子関係を持たない単独エンティティのみを `TransformSystem` の担当とし、親子関係を持つエンティティのワールド行列を保護した。

```cpp
// 修正前: 全エンティティの行列を上書き
auto entities = registry.view<TransformComponent>();

// 修正後: 親子関係を持つエンティティを排他除外
auto entities = registry.view<TransformComponent>(entt::exclude<HierarchyComponent>);
```

---

### 2.4. `Time.h` 新設に伴う C標準CRT `time.h` とのヘッダー名衝突 (C2039)

*   **現象 (Symptom)**:
    デルタタイムアクセサ `Time.h` を作成後、ビルド時に `<ctime>` 内部で `error C2039: 'clock_t': '`global namespace'' のメンバーではありません` という原因不明のコンパイルエラーが発生した。
*   **原因 (Root Cause)**:
    Windows（NTFS）は大文字小文字を区別しないため、標準ライブラリの `<chrono>` や `<ctime>` が内部で `#include <time.h>` を呼んだ際、CRT標準の `time.h` ではなくプロジェクト直下の `Time.h` が誤認されてインクルードされていた。
*   **解決策 (Resolution)**:
    ヘッダーファイル名を標準ヘッダーと衝突しない **`GameTime.h`** に改名。クラス名（`Engine::GameTime`）はそのまま維持し、安全なインクルードを確立した。

---

## 3. 最終実装ソースコード

### 3.1. コンポーネント定義
[`Components.h`](file:///d:/DirectX11_Sample/DirextX11Engine/Components.h#L23-L31)
```cpp
namespace Engine
{
    struct TransformComponent
    {
        DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4X4 worldMatrix;

        bool isDirty = true;
    };
```

[`Components.h`](file:///d:/DirectX11_Sample/DirextX11Engine/Components.h#L169-L177)
```cpp
    struct HierarchyComponent
    {
        entt::entity parent = entt::null;
        entt::entity firstChild = entt::null;
        entt::entity nextSibling = entt::null;
        entt::entity prevSibling = entt::null;

        uint32_t depth = 0; // ルートからの階層深度
    };
}
```

### 3.2. 階層トランスフォームシステム
[`HierarchyTransformSystem.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/HierarchyTransformSystem.cpp#L1-L84)
```cpp
#include "HierarchyTransformSystem.h"

namespace Engine
{
    void HierarchyTransformSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<TransformComponent>();

        for (auto [entity, transformComp] : view.each())
        {
            HierarchyComponent* hierarchyComp = registry.try_get<HierarchyComponent>(entity);

            // ルートノードのみ処理開始 (parent が null のもの)
            if (!hierarchyComp || hierarchyComp->parent == entt::null)
            {
                DirectX::XMMATRIX worldMat;
                if (transformComp.isDirty)
                {
                    worldMat = ComputeLocalMatrix(transformComp);
                    DirectX::XMStoreFloat4x4(&transformComp.worldMatrix, worldMat);
                }
                else
                {
                    worldMat = DirectX::XMLoadFloat4x4(&transformComp.worldMatrix);
                }

                // 子ノードへ再帰伝播 (Dirtyフラグ伝播)
                if (hierarchyComp && hierarchyComp->firstChild != entt::null)
                {
                    UpdateSubtree(registry, hierarchyComp->firstChild, worldMat, transformComp.isDirty);
                }

                transformComp.isDirty = false;
            }
        }
    }

    void HierarchyTransformSystem::UpdateSubtree(
        entt::registry& registry,
        entt::entity current,
        const DirectX::XMMATRIX& parentWorldMatrix,
        bool parentIsDirty)
    {
        entt::entity curr = current;
        while (curr != entt::null && registry.valid(curr))
        {
            auto& childTransform = registry.get<TransformComponent>(curr);
            auto& childHierarchy = registry.get<HierarchyComponent>(curr);

            // 親がDirty、または子自身がDirtyの場合に再計算
            bool needUpdate = childTransform.isDirty || parentIsDirty;
            DirectX::XMMATRIX childWorldMat;

            if (needUpdate)
            {
                DirectX::XMMATRIX childLocalMat = ComputeLocalMatrix(childTransform);
                // 子ワールド行列 = 子ローカル行列 * 親ワールド行列 (DirectXMath 行優先規約)
                childWorldMat = DirectX::XMMatrixMultiply(childLocalMat, parentWorldMatrix);
                DirectX::XMStoreFloat4x4(&childTransform.worldMatrix, childWorldMat);
            }
            else
            {
                childWorldMat = DirectX::XMLoadFloat4x4(&childTransform.worldMatrix);
            }

            // 子孫ノードへ再帰 (needUpdate を確実に伝播)
            if (childHierarchy.firstChild != entt::null)
            {
                UpdateSubtree(registry, childHierarchy.firstChild, childWorldMat, needUpdate);
            }

            childTransform.isDirty = false;
            curr = childHierarchy.nextSibling;
        }
    }

    DirectX::XMMATRIX HierarchyTransformSystem::ComputeLocalMatrix(const TransformComponent& transform)
    {
        DirectX::XMVECTOR s = DirectX::XMLoadFloat3(&transform.scale);
        DirectX::XMVECTOR r = DirectX::XMLoadFloat4(&transform.rotation);
        DirectX::XMVECTOR t = DirectX::XMLoadFloat3(&transform.position);

        return DirectX::XMMatrixAffineTransformation(s, DirectX::XMVectorZero(), r, t);
    }
}
```

### 3.3. 階層関係操作サービス
[`HierarchyService.h`](file:///d:/DirectX11_Sample/DirextX11Engine/HierarchyService.h#L1-L88)
```cpp
#pragma once
#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
    class HierarchyService
    {
    public:
        // 循環参照検出
        static bool IsAncestorOf(entt::registry& registry, entt::entity potentialAncestor, entt::entity target)
        {
            if (potentialAncestor == target || potentialAncestor == entt::null || target == entt::null)
                return false;

            entt::entity curr = target;
            while (curr != entt::null && registry.valid(curr))
            {
                auto* h = registry.try_get<HierarchyComponent>(curr);
                if (!h || h->parent == entt::null) break;
                if (h->parent == potentialAncestor) return true;
                curr = h->parent;
            }
            return false;
        }

        // 親子関係の設定
        static bool SetParent(entt::registry& registry, entt::entity child, entt::entity newParent)
        {
            if (child == newParent || child == entt::null) return false;
            if (newParent != entt::null && IsAncestorOf(registry, child, newParent)) return false;

            Unparent(registry, child);
            if (newParent == entt::null) return true;

            auto& childH = registry.get_or_emplace<HierarchyComponent>(child);
            auto& parentH = registry.get_or_emplace<HierarchyComponent>(newParent);

            childH.parent = newParent;
            childH.depth = parentH.depth + 1;
            childH.nextSibling = parentH.firstChild;
            childH.prevSibling = entt::null;

            if (parentH.firstChild != entt::null && registry.valid(parentH.firstChild))
            {
                auto& firstSiblingH = registry.get<HierarchyComponent>(parentH.firstChild);
                firstSiblingH.prevSibling = child;
            }
            parentH.firstChild = child;

            if (auto* t = registry.try_get<TransformComponent>(child))
                t->isDirty = true;

            return true;
        }

        // 親子関係の解除
        static void Unparent(entt::registry& registry, entt::entity child)
        {
            auto* childH = registry.try_get<HierarchyComponent>(child);
            if (!childH || childH->parent == entt::null) return;

            auto* parentH = registry.try_get<HierarchyComponent>(childH->parent);
            if (parentH && parentH->firstChild == child)
            {
                parentH->firstChild = childH->nextSibling;
            }

            if (childH->prevSibling != entt::null && registry.valid(childH->prevSibling))
            {
                auto& prevH = registry.get<HierarchyComponent>(childH->prevSibling);
                prevH.nextSibling = childH->nextSibling;
            }
            if (childH->nextSibling != entt::null && registry.valid(childH->nextSibling))
            {
                auto& nextH = registry.get<HierarchyComponent>(childH->nextSibling);
                nextH.prevSibling = childH->prevSibling;
            }

            childH->parent = entt::null;
            childH->prevSibling = entt::null;
            childH->nextSibling = entt::null;
            childH->depth = 0;

            if (auto* t = registry.try_get<TransformComponent>(child))
                t->isDirty = true;
        }
    };
}
```

---

## 4. 今後の拡張への指針

1. **トポロジカルソート（Topological Sort）による反復化**:
   現在の再帰呼び出し（`UpdateSubtree`）でもスタック深度は十分に安全ですが、階層深度（`depth`）順にエンティティ配列をソートしておくことで、完全なフラット反復（$O(N)$ ループ、スタック消費ゼロ）に置き換えることが可能です。
2. **モデルボーン（スケルトン）とのハイブリッド連携**:
   キャラクターの手に武器を持たせる（Socketアタッチ）場合、特定のボーンの `GlobalTransformMatrix` を親のワールド行列として取得し、武器エンティティの `TransformComponent` と合成するシステム間連携がスムーズに構築可能です。

---

## 5. 専門用語集

1. **排他クエリ (`entt::exclude`)**:
   EnTTのビューにおいて、特定のコンポーネントを持つエンティティを処理対象から除外する機能。複数システムの処理衝突を防ぐために使用する。
2. **ヘッダー名衝突 (Header Name Collision)**:
   大文字小文字を区別しないWindowsファイルシステム等において、自作ファイル名がCRT標準ヘッダー（`time.h` 等）と被ることで生じるインクルード誤認トラブル。
3. **Dirtyフラグ伝播 (Dirty Propagation)**:
   上位ノードの変位（Dirty）を下位ノードへと引き継ぎ、実際に姿勢が変わったノード群のみを行列計算の対象にする最適化手法。
