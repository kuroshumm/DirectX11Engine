# プロジェクト共通ルール: DirectX11 演習エンジン

本ドキュメントは、このワークスペースでエージェントが従うべき共通ルールを定めたものです。

---

## 1. 演習ドキュメントのライフサイクル管理ルール（インクリメンタル記録・完了時作成・セッション復元）

### 1.1. ドキュメント管理の目的と基本原則
単一のチャットセッションで演習の全行程を完結させようとすると、コンテキスト長（会話履歴）の肥大化に伴いLLMの推論精度低下や忘却（Context Rot）が発生します。
これを回避し、セッションを安全に分割・再開できるようにするため、ドキュメントを外部状態ストレージ（チェックポイント）として活用します。

ただし、**ユーザーの質問に回答するたびに毎回ドキュメントを更新することは禁止**します（ツールのオーバーヘッドによる応答遅延と、試行錯誤の中間状態によるドキュメントの破壊・スラッシングを防止するため）。
ドキュメントの更新は、以下の「イベント駆動」でのみ実施します。

### 1.2. 更新トリガー（イベント駆動ルール）
1.  **イベントA: 不具合・技術的課題の解決時（インクリメンタル追記）**:
    *   コンパイルエラー、描画崩壊、座標系の不整合などの原因が特定・修正された段階で、`Document/Ex[演習番号]_[テーマ名]_Implementation_Report.md` の「発生した主要な不具合と解決の軌跡」に「現象」「原因」「解決策」の3要素を即時追記する。
2.  **イベントB: セッション中断・切り替え時（チェックポイント保存）**:
    *   会話が長期化した場合や、ユーザーから「セッションを切り替える」「現状をセーブして」との指示があった場合、現在の進捗・確定事項・残タスクをドキュメントに記録する。
3.  **イベントC: 演習完了フェーズ（全体統合・最終化）**:
    *   プログラムのビルドが通り、機能が正常に動作し、検証がすべてパスした段階で、ユーザーの合意を得た上で `Architecture_Design.md`（システム構造・シーケンス図・数学的モデル）の新規作成、および `Implementation_Report.md` の全体整形・仕上げを行う。

### 1.3. 新規セッション開始時のセッション復元（Context Rehydration）義務
新しいチャットセッションで演習を再開する際、エージェントは指示を待たず自発的に `Document/` ディレクトリ内の該当演習のドキュメント（`ExXX_...md`）を `view_file` で読み込み、直前の到達点・解決済み不具合・確定設計を把握した上で、ユーザーへの回答やレビューを開始しなければならない。

### 1.4. ドキュメントの配置場所と命名規則
*   **配置ディレクトリ**: 全てのドキュメントは、プロジェクトルート直下の `Document` ディレクトリ内に作成すること。
*   **ファイル名**:
    *   設計仕様書: `Document/Ex[演習番号]_[テーマ名]_Architecture_Design.md`
    *   トラブルシューティング報告書: `Document/Ex[演習番号]_[テーマ名]_Implementation_Report.md`
*   ※ `[演習番号]` は2桁（例: 01, 19, 20）で記述すること。

### 1.5. ドキュメント作成スキルの適用
*   ドキュメントの作成・追記を行う際は、必ず `exercise_documenter` スキル ([SKILL.md (exercise-documenter)](file:///d:/DirectX11_Sample/DirextX11Engine/.agent/skills/exercise-documenter/SKILL.md)) を読み込み、そこに定義されている構成要件・命名規則・記述ルールを厳格に遵守すること。

### 1.6. 記述の厳密性
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


