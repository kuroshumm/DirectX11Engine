# フリーフロー・コンバット フェーズ1 詳細実装計画書
## （シーン再構築・プリミティブ生成・プレイヤー移動・TPSカメラ）

---

## 1. 概要と目的 (Overview & Objectives)

本ドキュメントは、[Document/FreeflowCombat_Roadmap.md](file:///d:/DirectX11_Sample/DirextX11Engine/Document/FreeflowCombat_Roadmap.md) における「**Phase 1: シーン再構築 ＋ プレイヤー移動 ＋ TPSカメラ**」を完全に達成するための詳細な実装計画書である。

これまでに策定された全体ロードマップ（詳細設計版）の Phase 1 要件と、地面描画のために設計・着手された「Unity風プリミティブオブジェクト生成システム（`GeometryGenerator`, `PrimitiveMeshCache`, `PrimitiveFactory`）」を統合し、単一の明確な実装計画として定義する。

### 1.1. 解決するギャップ一覧

| 識別子 | ギャップ項目 | 重要度 | 解決アプローチ |
|---|---|---|---|
| **G1** | **シーン構成の変更** | 必須 | `App::InitGeometry` 内の 500 体ランダム生成を撤去し、プレイヤー 1 体 ＋ 地面プレーン ＋ デバッグ用プリミティブの構成に移行する。 |
| **G2** | **可視的な地面メッシュ** | 必須 | Unityの `GameObject.CreatePrimitive(PrimitiveType::Plane)` 相当の基盤を構築し、50m × 50m の地面を $(0, 0, 0)$ に生成・描画する。 |
| **G3** | **プレイヤー識別** | 必須 | エンティティからプレイヤーを一意にクエリできる軽量なタグコンポーネント `PlayerTag` を定義・付与する。 |
| **G4** | **カメラ相対の移動制御** | 必須 | TPSカメラの水平前方・右方ベクトルを基準に WASD 移動ベクトルを算出し、キャラクターを移動方向へクォータニオン Slerp で滑らかに旋回させつつ、速度に応じて `Idle` $\leftrightarrow$ `Walk` $\leftrightarrow$ `Run` をクロスフェード再生する。 |
| **G5** | **三人称追従カメラ (TPS)** | 必須 | プレイヤーを注視点とするオービットカメラコンポーネント `ThirdPersonCameraComponent` と、マウスデルタ入力から球面座標系でカメラ位置を算出する `ThirdPersonCameraSystem` を実装する。 |

---

## 2. システムアーキテクチャ (Architecture Overview)

フェーズ1で構築するサブシステム群、およびアクション調停パイプラインのアーキテクチャを以下に示す。

![フェーズ1 システムアーキテクチャ](file:///d:/DirectX11_Sample/DirextX11Engine/Document/Images/phase1_pipeline_architecture.png)

### 2.1. レイヤー責務とデータ所有権 (Data Ownership)

パイプラインの各層は明確な関心事の分離（Separation of Concerns: SoC）と単一書き込み権限（Single Writer Principle）に基づいて設計されている。

| レイヤー | 主要システム | 主な管理データ（Writer） | 存在意義と責務（ニュアンス） |
|---|---|---|---|
| **1. 入力層 (Input Layer)** | `PlayerInputSystem` | `PlayerInputComponent` | **【ハードウェア入力の抽象化】**<br/>OS（Win32 RAWINPUT）のキー・マウス信号を、ゲームロジック非依存の正規化入力軸（`moveAxis`）および先行入力バッファ（`bufferAction`）に変換する。 |
| **2. 要求層 (Proposal Layer)** | `PlayerMovementSystem`<br/>`CombatSystem` | `LocomotionProposalComponent`<br/>`CombatRequestComponent` | **【意図 (What) の生成・文脈結合】**<br/>カメラ視線方向や索敵ターゲットと入力を結合し、「ワールドでどう動きたいか」「誰を攻撃したいか」の意図を生成する。身体制約・硬直・受付判定（Can）は持たない。 |
| **3. 状態調停層 (Arbitration Layer)** | `CharacterActionStateSystem` | `CharacterActionStateComponent` | **【可否 (Can) の一元的裁定と競合調停】**<br/>相反する要求と現在のキャラクター状態（硬直、`COMBO_QUEUE` 区間等）を突き合わせ、次フレームの全体状態（`ActionState`）と先行予約を一元的に裁定する。 |
| **4. 状態別実行層 (State Execution Layer)** | `LocomotionStateSystem`<br/>`AttackingStateSystem` | `CharacterActionResultComponent`<br/>`CharacterLocomotionComponent` | **【状態固有ドメインロジックの実行】**<br/>確定した単一の `ActionState` のシステムのみが動作し、移動サブステート（Idle/Walk/Run）の決定、コンボ技の解決（COMBO_1 → COMBO_2）、目標速度・目標旋回・再生アニメを出力する。 |
| **5. 結果反映層 (Action Applier Layer)** | `ActionApplierSystem` | `VelocityComponent`<br/>`TransformComponent` (旋回)<br/>`AnimatorSystem` | **【単一反映役 (Single Writer) / エンジンAPIへの適用】**<br/>実行層が出力した抽象結果を、物理速度のセット、Slerp によるクォータニオン旋回補間、アニメーションの CrossFade 切り替えなどの実体へ一括反映する。 |
| **6. 物理積分層 (Physics Layer)** | `MovementSystem` | `TransformComponent` (位置) | **【時間積分と環境拘束の維持】**<br/>ゲームロジックから独立し、確定した速度から位置積分（`position += velocity * dt`）を行い、フロアクランプ（`Y = 0` 接地維持）や壁面衝突を解決する。 |
| **7. 描画層 (Rendering Layer)** | `RenderSystem`<br/>`CullingSystem` | GPU 定数バッファ | **【視覚化とGPU描画】**<br/>視錐台カリングを経て、静的プリミティブ（地面）およびスキニングモデル（プレイヤー）の PBR 描画パイプラインを実行する。 |

### 2.2. 要求・提案層と状態調停層の存在意義（What と Can の分離）

本アーキテクチャにおいて「要求・提案層」と「状態調停層」を厳密に分離している理由は以下の通りである。

1. **生入力からゲーム空間上の意図（What）へのセマンティック変換**:
   * 入力層にある生データ（WASDキー押下など）を、カメラの水平基底ベクトルと合成して「3次元ワールド空間の移動ベクトル」に翻訳する（`PlayerMovementSystem`）。
   * 単なるボタン入力を、周囲の敵の索敵・ロックオン候補と結合して「誰に対する攻撃意図か」に翻訳する（`CombatSystem`）。
2. **多重入力源（プレイヤー・敵AI・通信同期）の抽象化**:
   * 人間プレイヤー（`PlayerMovementSystem`）だけでなく、将来の敵NPC（`EnemyAIMovement`）や通信同期も全く同じ要求コンポーネントを出力先とする。調停層以降は「誰が要求したか」を気にせず一律に処理できる。
3. **「何をしたいか (What)」と「今できるか (Can)」の境界分離**:
   * 要求層（意思決定主）は「攻撃したい」「走りたい」という意図（What）のみを表明し、アニメーションの再生タイムラインやイベント区間（`COMBO_QUEUE` など）は見ない。
   * 「今キャラクターがその要求を受け入れられるか（Can）」は、キャラクターの状態とアニメーション再生コンテキストを所有する状態調停層（`CharacterActionStateSystem`）が一元的に裁定し、先行入力予約や破棄を行う。これにより、アニメーション仕様の変更が入力・AIロジックへ波及（漏出）するのを防ぐ。

---

## 3. サブシステム1: 静的プリミティブ生成・レンダリング基盤 (G2 解決)

### 3.1. 静的頂点シェーダーと入力レイアウトの分離

*   **課題**: 既存の `PBRVertexShader.hlsl` はキャラクター用の `SkinnedVertex`（92バイト：ボーンID・ウェイト含む）固定となっており、地面やキューブなどの静的ジオメトリをそのまま描画できない。
*   **解決策**:
    1.  静的メッシュ用の頂点シェーダー [`Shaders/StaticPBRVertexShader.hlsl`](file:///d:/DirectX11_Sample/DirextX11Engine/Shaders/StaticPBRVertexShader.hlsl) を導入する。
    2.  `Vertex` 構造体（48バイト：Position, Normal, Tangent, Color, TexCoord）に対応する `ID3D11InputLayout` を作成する。
    3.  既存の [`PBRPixelShader.hlsl`](file:///d:/DirectX11_Sample/DirextX11Engine/Shaders/PBRPixelShader.hlsl) と同一の `PS_INPUT` 構造体を出力することで、PBRライティングパイプライン（金属度、粗さ、ディレクショナルライト計算）を共用する。

#### HLSL 頂点シェーダーの入出力定義
```hlsl
// 入力: 静的頂点 (48バイト)
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};

// 出力: ピクセルシェーダー入力 (PBRPixelShader.hlsl と完全一致)
struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};
```

### 3.2. 形状生成エンジン ([GeometryGenerator](file:///d:/DirectX11_Sample/DirextX11Engine/Model/GeometryGenerator.h))

数学的アルゴリズムに基づき、頂点バッファ・インデックスバッファの元データを生成する。

```cpp
namespace Engine
{
    struct MeshData
    {
        std::vector<Vertex> Vertices;
        std::vector<WORD>   Indices;
    };

    class GeometryGenerator
    {
    public:
        // 平面（地面）: 幅, 奥行き, グリッド分割数 m×n, UVタイリング数
        static MeshData CreatePlane(float width, float depth, uint32_t m, uint32_t n, float uvTilingX, float uvTilingY);
        // 立方体: 幅, 高さ, 奥行き
        static MeshData CreateCube(float width, float height, float depth);
        // 球体: 半径, スライス数, スタック数
        static MeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
        // 円柱: 下底半径, 上底半径, 高さ, スライス数, スタック数
        static MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);
    };
}
```

*   **UVタイリング対応**: 地面プレーン（50m×50m）において、テクスチャが引き伸ばされるのを防ぐため、`uvTilingX = 50.0f`, `uvTilingY = 50.0f` 等のタイリング引数をサポートする。
*   **バウンディングスフィア算出**: 生成した頂点配列の `Pos` 要素から `DirectX::BoundingSphere::CreateFromPoints` を呼び出し、視錐台カリング用の局所境界球を正確に初期化する。

### 3.3. 形状別リソースキャッシュ ([PrimitiveMeshCache](file:///d:/DirectX11_Sample/DirextX11Engine/Model/PrimitiveMeshCache.h))

プリミティブエンティティを複数生成する際、形状ごとに頂点バッファ・インデックスバッファ（`ModelAsset`）を重複生成しないようキャッシュ管理する。

```cpp
namespace Engine
{
    class PrimitiveMeshCache
    {
    public:
        std::shared_ptr<ModelAsset> GetOrCreatePlane(ID3D11Device* pDevice, float width, float depth, uint32_t m, uint32_t n, float uvTilingX, float uvTilingY);
        std::shared_ptr<ModelAsset> GetOrCreateCube(ID3D11Device* pDevice, float width, float height, float depth);
        std::shared_ptr<ModelAsset> GetOrCreateSphere(ID3D11Device* pDevice, float radius, uint32_t sliceCount, uint32_t stackCount);
        std::shared_ptr<ModelAsset> GetOrCreateCylinder(ID3D11Device* pDevice, float bottomRadius, float topRadius, float height, uint32_t sliceCount, uint32_t stackCount);

    private:
        std::shared_ptr<ModelAsset> CreateModelAssetFromMeshData(ID3D11Device* pDevice, const MeshData& meshData);

        std::unordered_map<std::string, std::shared_ptr<ModelAsset>> m_cache;
    };
}
```

### 3.4. Unity風エンティティ生成ファクトリ ([PrimitiveFactory](file:///d:/DirectX11_Sample/DirextX11Engine/Model/PrimitiveFactory.h))

Unity の `GameObject.CreatePrimitive` 相当の 1 行呼び出しインターフェースを提供する。

```cpp
namespace Engine
{
    enum class PrimitiveType
    {
        Plane,
        Cube,
        Sphere,
        Cylinder
    };

    class PrimitiveFactory
    {
    public:
        static void Initialize(
            Graphics* pGfx,
            std::shared_ptr<Shader> pStaticShader,
            std::shared_ptr<Texture> pDefaultTexture
        );

        static entt::entity CreatePrimitive(
            entt::registry& registry,
            PrimitiveType type,
            const DirectX::XMFLOAT3& position = { 0.0f, 0.0f, 0.0f },
            const DirectX::XMFLOAT3& scale = { 1.0f, 1.0f, 1.0f },
            const DirectX::XMFLOAT4& baseColor = { 1.0f, 1.0f, 1.0f, 1.0f }
        );
    };
}
```

---

## 4. サブシステム2: シーン再構築とプレイヤー識別 (G1, G3 解決)

### 4.1. シーン再構築（[`App::InitGeometry`](file:///d:/DirectX11_Sample/DirextX11Engine/App/App.cpp) の改修）

*   **既存ループの撤去**: 乱数で 500 体をばら撒く `for (int i = 0; i < 500; ++i)` ループを削除する。
*   **地面プレーンの生成**:
    ```cpp
    // 50m × 50m の地面を原点に配置
    PrimitiveFactory::CreatePrimitive(
        m_registry,
        PrimitiveType::Plane,
        DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),
        DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f) // 視認性の高いダークグレー
    );
    ```
*   **プレイヤーの生成**:
    - 原点 $(0, 0, 0)$ に SwordMan モデルのエンティティを 1 体のみ生成。
    - `TransformComponent`（初期座標 $0, 0, 0$）、`ModelAsset`（SwordMan）、`AnimatorComponent` を付与。
    - `PlayerTag` を付与。

### 4.2. プレイヤー識別タグコンポーネント

```cpp
// ECS/Components.h
struct PlayerTag {};
```

---

## 5. サブシステム3: 三人称追従カメラ (TPS) (G5 解決)

### 5.1. コンポーネント設計

既存のデバッグ用フリーカメラ（`FreeLookCameraControllerComponent`）から独立させ、三人称視点専用の追従コンポーネントを定義する。

```cpp
// ECS/Components.h
struct ThirdPersonCameraComponent
{
    entt::entity targetEntity = entt::null; // 追従対象（PlayerTagを持つエンティティ）
    
    float armLength    = 6.0f;  // カメラと注視点の距離 (メートル)
    float heightOffset = 1.6f;  // プレイヤーの足元からの注視点高さ（胸〜頭の高さ）
    
    float pitch = 0.25f;        // 垂直仰角 (rad) - 約 15° 見下ろし
    float yaw   = 0.0f;         // 水平方位角 (rad)
    
    float minPitch = -0.5f;     // 見上げ限界 (約 -28°)
    float maxPitch =  1.3f;     // 見下ろし限界 (約 +75°)
    
    float rotateSensitivity = 0.003f; // マウス感度
    float zoomSensitivity   = 0.5f;   // ホイールズーム感度（将来拡張）
};
```

### 5.2. 球面座標系によるカメラ座標計算アルゴリズム

注視点座標 `P_focus` を基準とし、球面座標系 `(rho, theta, phi)` からデカルト座標系への変換を行う。

```math
// 注視点座標の算出
P_focus = P_player + (0, heightOffset, 0)^T

// 球面座標からカメラワールド座標の算出
P_cam = P_focus + (
    armLength * cos(pitch) * sin(yaw),
    armLength * sin(pitch),
    armLength * cos(pitch) * cos(yaw)
)^T
```

```cpp
// Camera/ThirdPersonCameraSystem.cpp
void ThirdPersonCameraSystem::Update(entt::registry& registry, const InputSystem& input, float deltaTime)
{
    auto view = registry.view<TransformComponent, CameraComponent, ThirdPersonCameraComponent>();
    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& tps = view.get<ThirdPersonCameraComponent>(entity);

        if (!registry.valid(tps.targetEntity)) continue;
        const auto& targetTransform = registry.get<TransformComponent>(tps.targetEntity);

        // 1. マウス入力によるピッチ・ヨー角の更新
        DirectX::XMFLOAT2 mouseDelta = input.GetMouseDelta();
        tps.yaw   += mouseDelta.x * tps.rotateSensitivity;
        tps.pitch -= mouseDelta.y * tps.rotateSensitivity;

        // ピッチ角のクランプ
        tps.pitch = std::clamp(tps.pitch, tps.minPitch, tps.maxPitch);

        // 2. 注視点座標
        DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&targetTransform.position);
        DirectX::XMVECTOR focusPos = DirectX::XMVectorAdd(targetPos, DirectX::XMVectorSet(0.0f, tps.heightOffset, 0.0f, 0.0f));

        // 3. 球面座標からカメラ位置の算出
        float x = tps.armLength * std::cos(tps.pitch) * std::sin(tps.yaw);
        float y = tps.armLength * std::sin(tps.pitch);
        float z = tps.armLength * std::cos(tps.pitch) * std::cos(tps.yaw);
        DirectX::XMVECTOR camPos = DirectX::XMVectorAdd(focusPos, DirectX::XMVectorSet(x, y, z, 0.0f));

        DirectX::XMStoreFloat3(&transform.position, camPos);

        // 4. カメラの向き（クォータニオン）の算出
        DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(focusPos, camPos));
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(camPos, focusPos, up);
        
        // viewMatrix の逆行列からカメラのワールド回転クォータニオンを抽出
        DirectX::XMVECTOR scale, rotQuat, trans;
        DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, DirectX::XMMatrixInverse(nullptr, viewMatrix));
        DirectX::XMStoreFloat4(&transform.rotation, rotQuat);
    }
}
```

---

## 6. サブシステム4: アクション状態調停パイプラインとキャラクター移動制御 (G4 解決)

### 6.1. 既存構造体・列挙型の採否と責務の再定義

直前のアーキテクチャ設計およびドメイン分析に基づき、既存コンポーネントの採否と責務を以下のように確定する。

| 構造体 / 列挙型名 | 採否 | 新アーキテクチャにおける役割・責務 |
|---|---|---|
| `CombatActionType` | **必要（存続）** | プレイヤーの「攻撃・防御意図（インテント）」および `CombatSystem` が発行する「戦闘要求（`CombatRequestComponent`）」の種別を表すドメイン型。 |
| `PlayerInputComponent` | **必要（存続・純化）** | ハードウェア入力をゲーム内の論理入力（カメラ基準移動軸 `moveAxis`、先行入力バッファ `bufferAction` / `actionBufferTimer`）に抽象化して保持する。単一書き込み権限（Single Writer Principle）は `PlayerInputSystem` のみが保持する。 |
| `CharacterMovementStateComponent` | **不要（廃止・分解）** | `isMoving`, `isSprinting` は `PlayerInputComponent` および `CharacterLocomotionComponent` と二重管理（State Duplication）になっており未代入バグの原因であるため廃止。`isGrounded` は物理/衝突判定用の将来コンポーネントへ分離する。 |
| `CharacterLocomotionComponent` | **必要（存続・責務限定）** | 全体状態が `LOCOMOTION` であるときの内部サブ状態（`IDLE`, `WALK`, `RUN`）を保持する階層型ステートマシン（HSM）のサブコンポーネント。 |

### 6.2. 新設するコンポーネント設計 (ECS)

```cpp
// ECS/Components.h

// 1. 移動提案（PlayerMovementSystem -> CharacterActionStateSystem / LocomotionStateSystem）
struct LocomotionProposalComponent
{
    DirectX::XMFLOAT3 desiredVelocity{ 0.0f, 0.0f, 0.0f }; // 希望移動速度
    float desiredYaw = 0.0f;                                // 希望向くべき水平角度 (rad)
    bool hasMoveInput = false;                              // 移動入力の有無
    bool isSprinting = false;                               // スプリント意図
};

// 2. 戦闘要求（CombatSystem -> CharacterActionStateSystem）
struct CombatRequestComponent
{
    CombatActionType actionType = CombatActionType::NONE;
    entt::entity targetEntity = entt::null;                 // 攻撃対象エンティティ
    bool isConsumed = false;                                // 調停により消費されたか
};

// 3. キャラクター大域アクション状態
enum class ActionState : uint8_t
{
    LOCOMOTION, // 移動・待機・走行
    ATTACKING,  // 攻撃中（コンボ・突進・打撃）
    DODGING,    // 回避中（無敵時間・ステップ）
    HIT_STUN    // 被弾硬直
};

struct CharacterActionStateComponent
{
    ActionState currentState = ActionState::LOCOMOTION;
    float stateTimer = 0.0f; // 状態滞在時間
};

// 4. アクション結果（各StateSystem -> ActionApplierSystem）
struct CharacterActionResultComponent
{
    DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f }; // 今フレーム適用する確定速度
    float targetYaw = 0.0f;                         // 今フレーム目標とするYaw角
    bool shouldRotate = false;                      // 旋回更新を行うか
    float turnSpeed = 10.0f;                        // 旋回追従速度
    
    std::string targetAnim = "Idle";                // 再生すべきアニメーション名
    float crossFadeDuration = 0.25f;                // クロスフェード時間
};
```

### 6.3. 6層パイプラインの調停・実行シーケンス

競合の根絶と単一書き込み権限を保証するため、毎フレーム以下のシーケンスで実行される。

![アクション調停・実行シーケンス](file:///d:/DirectX11_Sample/DirextX11Engine/Document/Images/action_arbitration_pipeline.png)

### 6.4. カメラ相対移動ベクトルの算出アルゴリズム

WASD の入力を、TPSカメラの水平前方・右方ベクトルに射影してワールド移動ベクトルを合成する。

```math
// カメラのクォータニオン回転から3次元前方・右方ベクトルを取得
Forward_3D = Rotate( (0, 0, 1), CameraRotation )
Right_3D   = Rotate( (1, 0, 0), CameraRotation )

// 水平面 (Y = 0) へ射影して正規化
FlattenedForward = Normalize( (Forward_3D.x, 0, Forward_3D.z) )
FlattenedRight   = Normalize( (Right_3D.x,   0, Right_3D.z) )

// ワールド入力移動ベクトル
MoveDir = Normalize( FlattenedForward * moveAxis.y + FlattenedRight * moveAxis.x )
```

### 6.5. 状態別実行とアクション適用（Slerp旋回・クロスフェード・物理積分）

#### 1. LocomotionStateSystem の責務
`CharacterActionStateComponent.currentState == ActionState::LOCOMOTION` のエンティティのみを処理する。
- `LocomotionProposalComponent` の入力に基づき、`CharacterLocomotionComponent` のサブステート（`IDLE`, `WALK`, `RUN`）を更新。
- サブステートに応じた移動速度（0.0 m/s, 3.5 m/s, 6.5 m/s）およびアニメーション（`Idle`, `Walk`, `Run`）を決定し、`CharacterActionResultComponent` に書き込む。

#### 2. ActionApplierSystem の責務
各 StateSystem が書き込んだ `CharacterActionResultComponent` を読み取り、実体へ反映する唯一の適用役。
- `VelocityComponent` に確定速度を設定。
- `TransformComponent.rotation` を目標 Yaw へ向けて Slerp 旋回補間。
- `AnimatorSystem::CrossFade` を呼び出してアニメーションを切り替える。

```math
// クォータニオンの球面線形補間 (Slerp)
q_current = Slerp( q_current, q_target, clamp(turnSpeed * dt, 0.0, 1.0) )
```

#### 3. MovementSystem の責務
物理積分と接地維持のみを担当する。
```math
// 位置積分
position = position + velocity * dt
position.y = 0.0 // フロアクランプ (接地維持)
```

---

## 7. 実装ステップと作業順序 (Step-by-Step Implementation Roadmap)

各コンポーネントの依存関係に従い、以下の 5 ステップで順次構築する。

![実装ステップと作業順序](file:///d:/DirectX11_Sample/DirextX11Engine/Document/Images/phase1_implementation_steps.png)

### Step 1: 静的PBRレンダリングパイプラインの開通
1. [`Shaders/StaticPBRVertexShader.hlsl`](file:///d:/DirectX11_Sample/DirextX11Engine/Shaders/StaticPBRVertexShader.hlsl) の作成・コンパイル設定確認。
2. [`Graphics/Shader.h`](file:///d:/DirectX11_Sample/DirextX11Engine/Graphics/Shader.h) / [`Graphics/Shader.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/Graphics/Shader.cpp) に `VertexLayoutType { Static, Skinned }` を導入し、静的頂点（48バイト）用入力レイアウト生成メソッドを実装。

### Step 2: プリミティブ生成エンジンとリソースキャッシュの実装
1. [`Model/GeometryGenerator.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/Model/GeometryGenerator.cpp) で `CreatePlane`, `CreateCube`, `CreateSphere`, `CreateCylinder` を実装。
2. [`Model/PrimitiveMeshCache.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/Model/PrimitiveMeshCache.cpp) を完成させ、`MeshData` から `SubMesh` および `BoundingSphere` を含む `ModelAsset` を生成・キャッシュするロジックを実装。
3. [`Model/PrimitiveFactory.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/Model/PrimitiveFactory.cpp) を完成させ、ECS エンティティの組み立て（`TransformComponent`, `ModelAsset`, `MaterialComponent`, `VisibleTag`）を完結させる。

### Step 3: シーン再構築と地面描画
1. [`App/App.cpp`](file:///d:/DirectX11_Sample/DirextX11Engine/App/App.cpp) で `StaticPBRVertexShader` をロードし、`PrimitiveFactory::Initialize` を実行。
2. `App::InitGeometry` 内の 500 体ランダム生成コードを撤去。
3. `PrimitiveFactory::CreatePrimitive` で 50m × 50m の地面を配置。
4. 原点 `(0, 0, 0)` に SwordMan プレイヤーを 1 体配置し、ビルドして地面の上にキャラクターが描画されることを確認。

### Step 4: TPS追従カメラの実装
1. [`ECS/Components.h`](file:///d:/DirectX11_Sample/DirextX11Engine/ECS/Components.h) に `ThirdPersonCameraComponent` を定義。
2. [`Camera/ThirdPersonCameraSystem.h`](file:///d:/DirectX11_Sample/DirextX11Engine/Camera/ThirdPersonCameraSystem.h) / `.cpp` を新規作成し、マウス入力による球面座標系カメラ更新を実装。
3. カメラエンティティに `ThirdPersonCameraComponent` を設定（`targetEntity = player`）し、従来の `FreeLookCameraControllerComponent` を無効化。
4. マウス操作でプレイヤーの周囲を滑らかにカメラが旋回・追従することを確認。

### Step 5: アクション状態調停＆移動パイプラインの実装
1. **既存コンポーネントの整理・リファクタリング**:
   - `CharacterMovementStateComponent` を削除（二重管理の撤廃）。
   - `PlayerInputSystem.cpp` の値コピーバグ（`auto` → `auto&`）を修正し、`PlayerInputComponent` を単一書き込み権限で運用。
2. **新規コンポーネントの定義**:
   - `LocomotionProposalComponent`（移動提案）
   - `CombatRequestComponent`（戦闘要求）
   - `CharacterActionStateComponent`（大域アクション状態：LOCOMOTION, ATTACKING...）
   - `CharacterActionResultComponent`（確定アクション結果：速度, Yaw, アニメ）
3. **提案・調停・実行・反映システムの分離実装**:
   - `PlayerMovementSystem`: 入力とカメラから `LocomotionProposalComponent` を生成。
   - `CharacterActionStateSystem`: 要求から `ActionState` の一元的調停を行う。
   - `LocomotionStateSystem`: `ActionState::LOCOMOTION` 時に動作し、サブステート（IDLE/WALK/RUN）を更新して `CharacterActionResultComponent` へ書き込む。
   - `ActionApplierSystem`: 結果を `VelocityComponent`、旋回 Slerp、`AnimatorSystem::CrossFade` へ一括反映。
   - `MovementSystem`: `VelocityComponent` を位置へ積分し、`Y = 0.0` のフロアクランプを維持。
4. `App::Update` に新パイプライン順序でシステムを登録。

---

## 8. 検証計画と合否判定基準 (Verification & Acceptance Criteria)

### 8.1. 自動ビルド検証
*   MSBuild コマンドによるビルド検証:
    ```powershell
    MSBuild.exe DirextX11Engine.sln /p:Configuration=Debug /p:Platform=x64
    ```
*   **合否基準**: コンパイルエラー 0 件、リンクエラー 0 件で正常終了すること。

### 8.2. 目視・操作検証項目 (チェックリスト)

| # | 検証項目 | 操作・手順 | 期待される合格動作 |
|---|---|---|---|
| **V1** | **地面メッシュ描画** | エンジン起動 | 原点中心に広大な平面（Plane）が表示され、PBRライティングによる陰影が反映されていること。 |
| **V2** | **デバッグプリミティブ** | 任意配置の Cube / Sphere 確認 | 歪みなく正確な法線・PBRスペキュラで描画されていること。 |
| **V3** | **TPSカメラ追従** | マウス移動 | マウス水平・垂直移動に合わせてカメラがプレイヤー周囲を旋回し、指定した仰角制限（minPitch〜maxPitch）でクランプされること。 |
| **V4** | **カメラ相対移動** | WASD キー入力 | 常に「画面手前 / 奥 / 左右」のカメラ視線基準でプレイヤーが進行すること。 |
| **V5** | **キャラクター旋回** | 進行方向の変更 | 急反転せず、進行方向へ向けて Slerp で滑らかに体を向けること。 |
| **V6** | **アニメーション遷移** | 移動 ↔ 停止、Shiftダッシュ | 停止（Idle）から移動で Walk、Shift押しで Run へと足滑りなくスムーズにクロスフェードし、キーを離すと Idle に復帰すること。 |
| **V7** | **接地維持** | 移動中の挙動観察 | プレイヤーの足元が常に地面（`Y = 0`）に接地し、上下に浮遊・沈下しないこと。 |

---

## 9. 専門用語集 (Glossary)

*   **単一書き込み権限の原則 (Single Writer Principle)**:
    特定のコンポーネントやデータフィールドに対する変更責任と権限を持つシステムを唯一に限定する設計原則。複数のシステムが同一データを競合して書き換えることによる順序依存バグや競合状態を防止する。
*   **信頼できる唯一の情報源 (SSOT / Single Source of Truth)**:
    システム内で同一の意味を持つデータが複数箇所に重複して存在せず、唯一の確定したデータ源を参照する設計原則。フラグの二重管理（State Duplication）による不整合を防ぐ。
*   **階層型ステートマシン (HSM / Hierarchical State Machine)**:
    状態の中にさらに子状態を持たせる構造。全体の大域状態が「移動（Locomotion）」のとき、その内部で「待機（Idle）」「歩行（Walk）」「走行（Run）」の下位サブ状態を安全に切り替える設計手法。
*   **直交性 (Orthogonality)**:
    二つ以上のコンポーネントやシステムが互いに依存せず、一方の変更や拡張が他方に副作用を与えない性質。「プレイヤーの入力インテント」と「キャラクターの現在状態」を分離することで直交性を担保する。
*   **球面座標系 (Spherical Coordinate System)**:
    空間上の位置を、原点からの距離（半径 `r` またはアーム長）、方位角（Yaw `theta`）、仰角（Pitch `phi`）の 3 つのパラメータで表す座標系。注視点を中心とする TPS オービットカメラの算出に最も適している。
*   **Slerp (Spherical Linear Interpolation / 球面線形補間)**:
    単位クォータニオン間を角速度一定で滑らかに回転補間する手法。急激な向きの切り替わりによるアニメーションの不連続・ガタつき（ジャーク）を防止する。
*   **オービットカメラ (Orbit Camera)**:
    特定のターゲット（キャラクター）を常に中心（注視点）に据え、その周囲の球面上を軌道（Orbit）周回する三人称視点カメラ制御方式。
*   **フロアクランプ (Floor Clamp)**:
    物理エンジン（重力・剛体シミュレーション）を導入せずに、毎フレームキャラクターの垂直座標を地表の高さ（`Y = 0`）に強制的に固定・制限する軽量な接地処理。
*   **PBR (Physically Based Rendering / 物理ベースレンダリング)**:
    光の物理的な反射・拡散特性（金属度 Metallic、粗さ Roughness）に基づき、現実世界の光学法則を近似して質感を描画するレンダリング手法。
