# フリーフロー・コンバット フェーズ1 実装トラブルシューティング報告書
## （シーン再構築・プリミティブ生成・プレイヤー移動・TPSカメラ）

---

## 第1章 演習の概要と達成目標

本演習（Freeflow Combat Phase 1）では、バットマン アーカムシリーズ風のフリーフロー・コンバットを実現するための第1マイルストーンとして、シーン再構築、Unity風プリミティブ生成基盤、プレイヤーのTPSカメラ追従およびカメラ相対移動の確立を目的とする。

---

## 第2章 発生した主要な不具合と解決の軌跡

### 2.1. 不具合1: C2864 `PrimitiveMeshCache::m_cache` のクラス内初期化子エラー
*   **現象**:
    `Model/PrimitiveMeshCache.h(20)` において、`エラー C2864: 'Engine::PrimitiveMeshCache::m_cache': in-class 初期化子を持つ静的 データ メンバー には、非揮発性 const 整数型が必要であるか、'inline' の指定が必要です` が発生しコンパイルに失敗した。
*   **原因**:
    C++の言語仕様により、クラス定義内（in-class）で静的データメンバ（`static`）に初期化子（`{}` や `= ...`）を持たせる場合、`const` 整数型または `constexpr` 型に限定されていた。`std::array<std::shared_ptr<ModelAsset>, ...>` は非整数クラス型であるため、単なる `static` では初期化構文が不正となった。
*   **解決策**:
    本プロジェクトは C++20（`/std:c++20`）でビルドされているため、C++17で導入された `inline` 修飾子を付与し、`inline static std::array<std::shared_ptr<ModelAsset>, TYPE_COUNT> m_cache{};` と宣言することで、ヘッダー内での直接初期化とODR（単一定義規則）遵守を両立させた。

---

### 2.2. 不具合2: LNK2019 `PrimitiveFactory::Initialize` の未解決外部シンボル
*   **現象**:
    `App.cpp` 内で `PrimitiveFactory::Initialize` を呼び出した際、リンカーエラー `LNK2019: 未解決の外部シンボル "public: static void __cdecl Engine::PrimitiveFactory::Initialize(...)" が関数 "App::Initialize" で参照されました` が発生した。
*   **原因**:
    プロジェクトファイル `DirextX11Engine.vcxproj` において、`Model\PrimitiveFactory.cpp` および `Model\PrimitiveMeshCache.cpp` のコンパイル設定に `<ExcludedFromBuild>true</ExcludedFromBuild>`（ビルドから除外）が指定されていた。このため、コンパイラが `.cpp` ファイルをコンパイルせず、オブジェクトファイル（`.obj`）が生成されず、リンカーがシンボルの実体を見つけられなかった。
    また、`PrimitiveFactory.h` で宣言されているプライベート静的メンバ変数（`m_pGfx`, `m_staticShader`, `m_defaultTexture`, `m_meshCache`）に実体定義が存在せず、潜在的な未解決シンボル（LNK2001）となっていた。

---

### 2.3. 不具合3: 静的メッシュ描画時の入力レイアウト・シェーダー不一致による描画消失
*   **現象**:
    `PrimitiveFactory::CreatePrimitive` で生成したキューブが表示されず、画面上に何も描画されなかった。
*   **原因**:
    `PrimitiveFactory::Initialize` にスキニング用のシェーダー（`PBRVertexShader.hlsl`）と92バイトの入力レイアウト（`CreateSkinnrdInputLayout`）が渡されていた。一方、キューブの頂点データは48バイトの `Vertex` 構造体であり、かつ描画時にボーンパレットバッファ（`b1` レジスタ）がバインドされていなかった。そのため、GPUの入力アセンブラ（IA）でオフセットの不整合が発生し、頂点シェーダー内でパレット行列演算の結果として頂点座標がゼロやNaN（非数）に潰れて描画消失していた。
*   **解決策**:
    静的メッシュ専用の頂点シェーダー `StaticPBRVertexShader.hlsl` をコンパイルし、48バイト用の静的レイアウト（`CreateStaticInputLayout`）を持つ独立したシェーダーインスタンス `m_pStaticPBRShader` を生成して `PrimitiveFactory::Initialize` に渡すことで正常描画を確立した。

---

### 2.4. 不具合4: マウス未操作時におけるカメラ追従スキップと初期構図の不整合
*   **現象**:
    アプリケーション起動直後、カメラが設定した所定の距離・角度（TPSオービット構図）にならず、プレイヤー（ターゲット）を至近距離でアップ表示してしまう。マウス入力を開始した瞬間に初めて所定のカメラ座標・角度へとスナップして切り替わる。
*   **原因**:
    `ThirdPersonCameraSystem::Update` のループ先頭において、`if (!inputSystem.TryGetMouseDelta(mouseDelta)) { continue; }` という早期脱落（Early Continue）ガード節が配置されていた。
    起動直後やマウスを静止させているフレームでは `mouseDelta` がゼロであるため `continue` され、後続の `CalcCameraPos` および `CalcCameraRotQuat` によるカメラ座標・回転の追従計算が一切実行されなかった。
    その結果、`App::InitPlayerCamera` で仮置きされた初期座標 `(0, 5, -15)` のまま描画され、ターゲットの眼前至近距離からアップで映されていた。また、この構造ではマウスを操作しない状態でプレイヤーが移動した際にカメラが追従しない潜在的不具合も内包していた。
*   **解決策**:
    「マウス入力による角度（pitch / yaw）の更新」と「注視点追従・カメラ座標/回転の計算」の制御フローを分離した。
    マウスデルタが存在する場合のみ角度を加算更新し、カメラのワールド座標（`CalcCameraPos`）および回転クォータニオン（`CalcCameraRotQuat`）の更新は、入力の有無にかかわらず毎フレーム無条件で実行するように制御構造を改修した。

---

### 2.5. 不具合5: LookAtLHの注視点不整合による極端な俯瞰角と頭部クリッピング
*   **現象**:
    カメラパラメータを典型的なTPSアクションゲーム向け（距離 `distance = 14.0f`、注視点高さ `heightOffset = 6.0f`、見下ろし角 `pitch = 0.18f`）に設定したにもかかわらず、画面の中心がキャラクターの足元（くるぶし付近）に固定され、キャラクターの頭部や上半身が画面上端外へ見切れて（クリップされて）しまい、画面下半分に地面のみが広く映る不自然な急角度の俯瞰構図となった。
*   **原因**:
    `ThirdPersonCameraSystem.cpp` 内において、カメラ位置算出（`CalcCameraPos`）ではターゲット位置に `heightOffset` を加算した注視点 `focusPos` を基準として球面座標オフセットを計算していた。
    しかし、カメラの視線方向（ビュー行列）を算出する `CalcCameraRotQuat` を呼び出す際、本来渡すべき注視点 `focusPos` ではなく、オフセット前の `targetPos`（足元のワールド座標 `Y = 0`）を第1引数に渡していた。
    この結果、カメラ位置が `Y ≈ 8.5`（頭部付近）に配置されているにもかかわらず、視線が `Y = 0`（足元）を直接注視するため、見下ろし角が設計値（約10度）の3倍以上である約32度へと強制的に偏向され、視錐台の中心軸が足元を捉えて上半身が画角外へ押し出されていた。
*   **解決策**:
    注視点の計算処理を `CalcFocusPos(targetTransform, tpsConfig)` として独立分離し、算出された共通の注視点ベクトル `focusPos` を `CalcCameraPos`（カメラ位置決定）および `CalcCameraRotQuat`（カメラ姿勢決定）の双方へ確実に引き渡す構造にリファクタリングした。これにより、カメラの光軸中心がキャラクターの胸・肩（`Y ≈ 6.0`）に正しく合致し、頭頂から足元までが画面の上下に収まる標準的な三人称構図を確立した。

