# プロジェクト共通ルール: DirectX11 演習エンジン

本ドキュメントは、このワークスペースでエージェントが従うべき共通ルールを定めたものです。

---

## 1. 演習完了時のドキュメント作成ルール

### 1.1. ドキュメント作成の義務
演習テーマに基づき、プログラムのビルドが通り、機能が正常に動作し、検証（またはテスト）がすべてパスした段階（演習完了フェーズ）において、エージェントは必ず以下の対応を行わなければなりません。

1.  **完了報告と同意取得**:
    *   実装が完了したことをユーザーに明確に報告する。
    *   演習記録のためのドキュメント（設計書・トラブルシューティング報告書）の作成を行う旨を提案し、ユーザーの合意を得る。
2.  **ドキュメント作成スキルの適用**:
    *   ユーザーの合意を得た後、必ず `exercise_documenter` スキル ([SKILL.md (exercise-documenter)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/exercise-documenter/SKILL.md)) を読み込み、そこに定義されている構成要件・命名規則・記述ルールを厳格に遵守してドキュメントを作成すること。

### 1.2. ドキュメントの配置場所と命名規則
*   **配置ディレクトリ**: 全てのドキュメントは、プロジェクトルート直下の `Document` ディレクトリ内に作成すること。
*   **ファイル名**:
    *   設計仕様書: `Document/Ex[演習番号]_[テーマ名]_Architecture_Design.md`
    *   トラブルシューティング報告書: `Document/Ex[演習番号]_[テーマ名]_Implementation_Report.md`

### 1.3. 記述の厳密性
*   ドキュメントの作成においては、適当なサンプルやプレースホルダーは一切使用せず、実装されたソースコードの正確な動きに基づき、Mermaid図やLaTeX数式を用いて厳密に記述すること。
*   バグやトラブルシューティングの記録（Implementation Report 内）では、「現象」「原因」「解決策」の3つの技術的要因を詳細かつ論理的に記載すること。

---

## 2. ミノ駆動インスパイア設計スキルの適用・活用ルール

**【自動発火原則】**
ユーザーから「特定機能の設計」「アーキテクチャの相談」「クラス/モジュール/データ構造の設計」「仕様の検討」について質問・依頼された場合、エージェントは指示を待たず自発的に以下の対応スキル (`.agent/skills/`) の `SKILL.md` を `view_file` で読込み、そこに定義されている判断基準 (Hard Gates) や成果物構造 (Outcome Contract) に従って回答・設計・検証を行わなければならない。

*   **問題定義・文脈整理 (Problem Framing)**:
    特定の機能の設計、仕様検討、前提整理、何を作るべきかの相談、または解くべき問題・成功条件が未整理な場合は、まず [SKILL.md (mino-problem-framing)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-problem-framing/SKILL.md) を参照する。
*   **ドメインモデル完全性検証 (Domain Model Completeness)**:
    ドメインモデル・データ構造の設計、状態遷移の検討、モデルの概念・不変条件・境界値の欠落を分析・検証する場合は [SKILL.md (mino-domain-model-completeness)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-domain-model-completeness/SKILL.md) を参照する。
*   **契約による設計 (Design by Contract)**:
    クラス設計、関数の事前条件・事後条件・不変条件の設計、失敗保証やテスト仕様の策定を行う場合は [SKILL.md (mino-design-by-contract)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-design-by-contract/SKILL.md) を参照する。
*   **インターフェースと実装の分離 (Interface Implementation Separation)**:
    インターフェース設計、カプセル化の徹底、内部技術の漏出防止、公開操作境界の検討を行う場合は [SKILL.md (mino-interface-implementation-separation)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-interface-implementation-separation/SKILL.md) を参照する。
*   **アーキテクチャ・品質戦略 (Architecture Quality Strategy)**:
    全体構成・アーキテクチャの相談、複数モジュール・データ所有権・品質トレードオフの設計を行う場合は [SKILL.md (mino-architecture-quality-strategy)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-architecture-quality-strategy/SKILL.md) を参照する。
*   **再現性ある統合開発 (Reproducible Development)**:
    中規模以上の機能開発・設計・実装・検証を統合的に推進する場合は [SKILL.md (mino-reproducible-development)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/mino-reproducible-development/SKILL.md) を参照する。


