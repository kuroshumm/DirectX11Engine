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

---

### 2.6. 不具合6: Mixamo FBX変換時におけるFixBind/ApplyRotation起因の空間不整合とボーン命名規則の変更
*   **現象**:
    Mixamoから取得したY-BotモデルにBlenderアドオン（Mixamo Rootbaker）でRootBoneを付加してDirectX11エンジンに読み込んだ際、メッシュパーツ（関節球と手足外殻）が空中に飛び散り、バインド姿勢（Tポーズ）において下半身が前方に90度折れ曲がって座り込むような重大な描画破綻が発生した。
*   **原因**:
    1. **メッシュパーツ間のノード回転空間の不一致**:
       実機から抽出した頂点バウンディングボックスおよびノード階層ログにより、`Joints`（関節球メッシュ）は回転なし（`Rot: (0,0,0,1)`、頂点 $Y \in [0.8, 159.8]$）の直立姿勢であったのに対し、`Surface`（ボディ外殻メッシュ）は $X$ 軸周りに90度回転（`Rot: (0.707107, 0, 0, 0.707107)`、頂点 $Z \in [-180.5, 0.0]$）の倒伏姿勢でエクスポートされていた。
    2. **逆バインド行列（$M_{offset}$）の単一キャッシュ適用**:
       Assimpの仕様上、$M_{offset}$（Inverse Bind Pose Matrix）は各メッシュのローカル空間を基準に算出されるため、`Joints`（Y-Up用）と `Surface`（Z-Up/90度回転補正用）で90度異なる行列が格納されていた。しかし、`ModelLoader.cpp` の `CalcGlobalBoneIndex` では先頭メッシュ（`Joints`）の $M_{offset}$ のみをボーン情報に登録し、後続の `Surface` にも同一の $M_{offset}$ を適用していた。その結果、倒れた生頂点に対して直立用の逆バインド行列が乗算され、90度回転補正が失われて座り込んだ姿勢になっていた。
    3. **Blenderアドオン設定（FixBind / ApplyRotation）の副作用**:
       Mixamo Rootbaker の「Fix Bind」や「Apply Rotation」がONになっていたことで、アーマチュアとメッシュオブジェクト間でトランスフォームの不均一なフリーズ・補正が発生していた。
*   **解決策**:
    1. **DCCツール側でのコンバート設定是正**:
       Blender側で「Fix Bind」および「Apply Rotation」のチェックを解除してエクスポートすることで、全メッシュ（`Joints`, `Surface`）とスケルトンのバインド空間が完全に統一された。
    2. **ボーン命名規則の更新とコード側同期**:
       再コンバートに伴い、ボーン命名規則がUnreal Engine形式（`Pelvis`, `thigh_l`, `calf_l`, `foot_l`）からMixamo標準命名規則（`root`, `Hips`, `LeftUpLeg`, `LeftLeg`, `LeftFoot`, `LeftToeBase`）へ移行した。
       これに対応するため、`App.cpp` 内のIKボーン検索処理（`findBone` ラムダ）において、`LeftUpLeg`（太もも）、`LeftLeg`（すね）、`LeftFoot`（足首）を優先検索しつつ旧名称へのフォールバックを備えた堅牢な検索機構へと改修した。また、アニメーションクリップ（`Idle`, `StandardWalk`, `Running`）の全ボーンチャンネルがモデル側と100%一致して同期再生されることを実機ログにより検証・確認した。

---

### 2.7. 不具合7: C++の型推論規則によるコンポーネント参照脱落とパイプライン更新消失
*   **現象**:
    `PlayerInputSystem::Update` において `PlayerInputComponent` の `moveAxis` に移動入力値を代入しているにもかかわらず、後続のシステム（`PlayerMovementSystem` や `LocomotionStateSystem`、`ActionApplierSystem`）に移動データが伝搬せず、プレイヤーが全く移動・回転しない現象が発生した。
*   **原因**:
    1. **`registry.get<T>` と `auto` による参照修飾子の剥離（Decay）**:
       EnTT の `registry.get<T>(entity)` は本来内部ストレージ内の実体への左辺値参照（`T&`）を返すが、呼び出し元で `auto playerInput = registry.get<PlayerInputComponent>(entity);` と記述していた。C++の型推論（Type Deduction）規則により、`auto` は型から参照修飾子 `&` を取り除いて推論するため、推論される型は `PlayerInputComponent`（値型）となる。その結果、スタック領域にローカルな複製（コピー）が生成され、このコピーに対して代入操作を行っても関数終了時に破棄され、Registry 内の実体には一切反映されていなかった。
    2. **`view.each()` における構造化束縛の値コピー**:
       `PlayerMovementSystem` や `CharacterActionStateSystem`、`LocomotionStateSystem`、`AttackingStateSystem`、`ActionApplierSystem`、`CombatSystem` などの各システムにおいて、`for (auto [entity, a, b] : view.each())` という構文で各コンポーネントを取り出していた。この構文では、タプルからアンパックされる各コンポーネントがすべて値コピー（複製）となるため、各システム内で計算結果（`locomotionProposal.velocity` や `charaActionResult`、`velocity.velocity`、`transform.rotation`）を代入しても Registry 上の実体コンポーネントが全く更新されていなかった。
    3. **移動入力軸（`CalcMoveDir`）の符号反転**:
       `PlayerInputSystem.cpp` の `CalcMoveDir` において、Wキー押下時に `moveDirLocal.y -= 1.0f`、Aキー押下時に `moveDirLocal.x += 1.0f` と記述されていた。3Dカメラ相対ベクトルとの合成演算（`cameraFoward * y + cameraRight * x`）において、前進方向（`+Z` 向きのカメラ前方ベクトル）に対して負の係数が掛かるため後退方向を向き、左右も反転する潜在的不整合が存在していた。
*   **解決策**:
    1. **明示的な参照修飾（`auto&` / `auto&&`）の徹底**:
       `registry.get<T>` を受け取る箇所を `auto& playerInput = registry.get<...>(...);` へ修正した。
    2. **`view.each()` での構造化束縛を参照受けへ統一**:
       全システムのループを `for (auto&& [entity, a, b] : view.each())` に変更し、Registry 内の各コンポーネントの実体を参照として安全にバインド・更新する構造へ統一した。
    3. **入力軸ベクトルの正規化と符号整合**:
       Wキーで `moveDirLocal.y += 1.0f`（前進）、Sキーで `moveDirLocal.y -= 1.0f`（後退）、Dキーで `moveDirLocal.x += 1.0f`（右移動）、Aキーで `moveDirLocal.x -= 1.0f`（左移動）に修正した。

---

### 2.8. 不具合8: AnimStateType導入に伴う循環インクルードと型不一致によるビルド遮断
*   **現象**:
    `AnimStateType` の導入および `AnimSequenceAsset` のデータ構造リファクタリングに伴い、コンパイル時に `C2065: 'AnimationClip': 定義されていない識別子です`、`C2653: 'AnimStateType': 識別子がクラス名でも名前空間名でもありません`、`C2665: 'find': オーバーロードされた関数ですべての引数の型を変換できませんでした` など計158件のエラーが発生し、ビルドが完全に遮断された。
*   **原因**:
    1. **循環インクルードとレイヤー違反（Circular Include & Layer Violation）**:
       低レイヤー基盤ヘッダーである `Core/Common.h` が `Core/LocomotionTypes.h` をインクルードし、それが `AnimStateType` を参照するためだけに高レイヤーの `ECS/Components/AnimationComponents.h` をインクルードしていた。一方で `AnimationComponents.h` は `AnimationClip.h` をインクルードし、`AnimationClip.h` は `Common.h` をインクルードしていた。この循環依存により、`AnimationClip.h` の定義完了前に `AnimSequenceAsset.h` が評価され、`AnimationClip` が未定義となった。また、`AnimSequenceAsset.h` 自身も `AnimStateType` の定義を持たずに参照していた。
    2. **新旧メンバ変数名・型の不整合**:
       `AnimSequenceAsset` のメンバ変数が `name`（`std::string`）から `animState`（`AnimStateType`）に変更されたが、`AnimSequenceLoader.cpp` 内で `asset->name = ...` への代入が残存していた。
    3. **`AnimatorSystem.cpp` でのポインタ・クリップ直接代入**:
       `AnimatorComponent::stateMap` のキー変更に伴う `stateMap.find` の引数型不一致、および `AnimationComponent::pCurrentClip`（`AnimationClip*`）に対して `pAnimSeq`（`AnimSequenceAsset*`）を直接代入・比較していたことによる型不整合（C2679 / C2446）。
*   **解決策**:
    1. **`AnimStateType.h` の独立分離**:
       `AnimStateType` を専用の軽量ヘッダー `Animation/AnimStateType.h` へ切り出し、`LocomotionTypes.h`、`AnimSequenceAsset.h`、`AnimationComponents.h` から参照させることで Core層から ECS層への逆流参照および循環インクルードを完全に断ち切った。
    2. **`AnimSequenceAsset.h` での前方宣言**:
       `class AnimationClip;` を前方宣言し、不要なヘッダー直接依存を最小化した。
    3. **`AnimSequenceLoader.cpp` および `AnimatorSystem.cpp` の型整合性修正**:
       `asset->animState = AnimStateType::...` への代入、および `it->second.pAnimSeq->clip` によるクリップ参照の統一を行い、全158件のエラーを解消してビルドを成功（0エラー）させた。

---

### 2.9. 不具合9: EnTTにおける同一コンポーネント二重登録（`Slot not available` アサーションエラー）
*   **現象**:
    アプリケーション起動時の `App::InitPlayer` 内 `m_registry->emplace<CharacterLocomotionComponent>(player);` において、EnTT 内部アサーション `Expression: ((elem == null) && ("Slot not available"))` が発生し、実行時クラッシュ（中断）に至った。
*   **原因**:
    1. **Factoryと初期化関数間での `emplace` の重複（Duplicate Component Emplacement）**:
       プレイヤーと敵の共通コンポーネント群を `CharacterFactory::SpawnHumanoid` 側へ集約した際、Visual Studio 側の未保存エディタバッファに旧 `App::InitPlayer` のコードが残存しており、ビルド実行時の自動保存によって `App::InitPlayer` 側の `m_registry->emplace<CharacterLocomotionComponent>(player);` 等の記述が復元された。
    2. **EnTT スパースセット（Sparse Set）の不変条件違反**:
       EnTT の `registry::emplace<T>(entity)` は、対象エンティティがまだ型 `T` のコンポーネントを所有していないことを事前条件（Precondition）としている。すでに `SpawnHumanoid` 内でスロットが確保・構築されたエンティティに対して再度 `emplace<T>` を呼び出したため、空きスロット検査 `elem == null` が偽となりアサーションが発火した。
*   **解決策**:
    `CharacterFactory::SpawnHumanoid` 側で付与済みの共通コンポーネント（`CharacterLocomotionComponent` 等11種）の `emplace` 呼び出しを `App::InitPlayer` から完全に撤去し、`App::InitPlayer` ではプレイヤー固有の `PlayerInputComponent` および `PlayerTag` のみを付与する構造へ単一化した。

