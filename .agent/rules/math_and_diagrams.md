# 数式および図表の表現制約 (Math & Diagram Formatting Constraints)

本ドキュメントは、チャット回答およびプロジェクト内ドキュメントにおいて、数式および図表の表示崩れ・生コード露出・視読性低下を防止するための**絶対遵守ルール（Hard Constraints）**を定めたものです。

---

## 1. 数式の表現制約（LaTeX・ドル記号の完全禁止）

Antigravity IDE のチャット環境および Markdown レンダラーには LaTeX レンダリング機能（KaTeX / MathJax 等）が存在しないため、**ドル記号（`$`）を用いた記法は一切機能せず、生コードが露出して表示崩壊を引き起こします。**

### 【絶対禁止事項 (FORBIDDEN)】
1. **ドル記号（`$` および `$$`）の一切の出力を禁止する（例外なき完全禁止）。**
   * インライン（`$...$`）だけでなく、ブロック（`$$...$$`）であっても、ドル記号は 1 文字たりとも出力してはならない。
2. **LaTeX 特有のマクロ記法（`\mathbf`, `\text`, `\begin{pmatrix}`, `\otimes`, `\times` 等）の平文出力を禁止する。**

### 【遵守事項 (MANDATORY)】
1. **インライン数式・変数・ベクトル・座標・クォータニオン**:
   * 必ずバッククォートを用いたインラインコード記法（`...`）を使用すること。
   * **BAD 例**:
     * `$v = (x, y, z)$`
     * `$\mathbf{q} = (q_x, q_y, q_z, q_w)$`
     * `$q \cdot v \cdot q^*$`
     * `4 \times 4`
   * **GOOD 例**:
     * `v = (x, y, z)`
     * `q = (qx, qy, qz, qw)`
     * `v' = q * v * q*`
     * `4x4`
2. **複数行の厳密な定義式・行列変換・アルゴリズム**:
   * 言語指定コードブロック（```math, ```cpp, ```hlsl）を使用すること。
   * **BAD 例**:
     ```latex
     $$
     \mathbf{v}' = \mathbf{q} \otimes \mathbf{v} \otimes \mathbf{q}^*
     $$
     ```
   * **GOOD 例**:
     ```math
     // クォータニオンによるベクトル回転 (サンドイッチ積)
     v' = q * (v, 0) * q*
     v_rot = v + 2 * cross(qv, cross(qv, v) + qw * v)
     ```
     または
     ```cpp
     // DirectXMath 実装
     DirectX::XMVECTOR vRot = DirectX::XMVector3Rotate(v, q);
     ```

---

## 2. 図表・ダイアグラムの表現制約（画像化義務）

### 【絶対禁止事項 (FORBIDDEN)】
1. **テキスト記号図（アスキーアート）の全面禁止**:
   * `+---+` や `|`, `-->` などのテキスト文字を組み合わせた簡易図は一切禁止する。
2. **Mermaid 生テキストブロック（```mermaid ... ```）の単体露出の禁止**:
   * チャット環境で自動描画されないため、コードブロックをそのまま放置してはならない。
3. **拡散モデル（画像生成AI）による設計図・ダイアグラム生成の禁止**:
   * 文字化けやノード破綻を起こすため、図解生成ツール（`generate_image`）を使用しないこと。

### 【遵守事項 (MANDATORY)】
1. **論理構造図・処理フロー・ECS 関係図**:
   * 必ず **Mermaid CLI (`npx -y @mermaid-js/mermaid-cli`)** を用いて高解像度 PNG 画像を出力し、Markdown リンク（`![caption](file:///...)`）として埋め込むこと。
2. **3次元幾何学図解（ベクトル・クォータニオン回転・カメラ視野）**:
   * Mermaid では表現できない連続空間図解は、必ず **SVG（Scalable Vector Graphics）** ファイルを生成し、画像リンク（`![caption](file:///...)`）として埋め込むこと。
3. **画像保存先**:
   * チャット一時図: アシスタントの scratch ディレクトリ（`<appDataDir>\brain\<conversation-id>\scratch\`）に保存する。
   * 恒久ドキュメント: `Document/Images/` 配下に保存する。
