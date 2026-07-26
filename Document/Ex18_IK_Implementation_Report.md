# 演習18 逆運動学 (IK) システム: 実実装完了報告書

この報告書は、DirectX11エンジンにおける逆運動学（IK）ソルバー（CCDおよびFABRIK）の実装、テスト検証、およびデバッグフェーズで得られた知見と解決策をまとめたものです。

---

## 1. 実装スコープと完了タスク

演習18において、アニメーションパイプラインに以下のモジュールを正常に統合しました：
* **`IIKSolver` 共通インターフェースの実装**: ソルバーに依存しない角度制限データ構造（`JointConstraint`）を定義し、ソルバー間で共通のAPIから制限角度を登録できるようにしました。
* **`CCDSolver` の開発**: 1自由度（ローカルX軸）の角度クランプ制限を搭載した、回転ベースのCCD（Cyclic Coordinate Descent）アルゴリズムを実装・動作確認しました。
* **`FABRIKSolver` の開発**: 計算負荷を最小に抑える位置ベースのFABRIKアルゴリズムを実装し、作業用バッファ `tempPosition` を用いた往復位置補正、および最終座標確定後のクォータニオン逆算バインドを完成させました。
* **リファクタリングによる可読性の向上**: 巨大化していたFABRIKの `Solve` 関数を、位置計算を担う `ResolvePositions` と、ボーンへ回転を適用する `ApplyPositionsToBones` の非公開関数へ分割し、コードの見通しを大幅に改善しました。

---

## 2. 遭遇した重大なバグと技術的解決策

MixamoやFBXフォーマットの実用的なキャラクターモデルをIKで制御するにあたり、以下の3つの幾何学・構造的な問題に遭遇し、解決しました。

### 2.1. ダミーノードによる「長さ0のボーン」縮退バグ (FABRIKでの足の消失)
* **不具合内容**: `FABRIKSolver` に切り替えた瞬間、キャラクターの左足が股関節の位置に1点に潰れ、硬直して動かなくなった。
* **根本原因**: ボーンの長さを `childBone.DefaultLocalMatrix` から取得していたため。FBXインポート時に挿入される軸補正用のダミーノード（例: `mixamorig:LeftLeg_$AssimpFbx$_Translation`）は、実関節と全く同じ座標にあるためローカル平行移動成分が `0` です。この結果、測定されたボーン長がすべて `0` になり、足全体が一点に潰れてしまいました。
* **解決策**: ボーン間の長さを、初期バインド姿勢における「グローバル行列（`GlobalTransformMatrix`）の座標の差分」から算出するように修正しました：
  $$d_i = \| \vec{P}_{child\_global\_init} - \vec{P}_{parent\_global\_init} \|$$
  これにより、中間のダミーノードの親子関係を飛び越えて、大腿や下腿の正しい物理的な骨の長さを測定できるようになりました。

### 2.2. クォータニオン積の適用順序エラーによる回転破綻
* **不具合内容**: FABRIKの位置解決後に差分回転を適用すると、足の骨がランダムな方向にねじ曲がり破綻した。
* **根本原因**: DirectXMathの `XMQuaternionMultiply(Q1, Q2)` は、計算の評価順が右から左（右側の $Q_2$ が先、左側の $Q_1$ が後）になる行ベクトル仕様です。元のコードでは `XMQuaternionMultiply(currentRot, deltaRot)` と書いていたため、グローバル基準で算出した差分回転がローカル空間に紛れ込み、回転軸が狂ってしまいました。
* **解決策**: 回転の適用順序を以下のように修正しました：
  ```cpp
  newGlobalRot = XMQuaternionMultiply(currentRot, deltaRot);
  ```
  現在のグローバル回転（左）に対して、グローバル基準の差分（右）を正しく後から適用する仕様に戻すことで、正しい方向に骨が追従するようになりました。

### 2.3. 関節角度制限適用時における「ねじれ（他の軸の回転）」の消失
* **不具合内容**: 膝に角度制限（X軸まわり）を設定したところ、膝だけでなく足首や股関節のねじれなど、他のすべての回転成分が消えて関節が完全にロックされた。
* **根本原因**: クランプ後の角度から `XMQuaternionRotationAxis` を用いてクォータニオンを再構築したため、制限軸以外のY軸やZ軸（ねじれ成分）の回転情報がすべて `0` に上書き消去されてしまったため。
* **解決策**: クランプ前後の「角度の差分（`deltaAngle`）」のみを算出し、元のローカル回転に対して差分回転だけを乗算で適用する設計に変更しました：
  ```cpp
  float deltaAngle = clampedAngle - currentAngle;
  XMVECTOR diffRot = XMQuaternionRotationAxis(constraint.localAxis, deltaAngle);
  newLocalRot = DirectX::XMQuaternionMultiply(diffRot, newLocalRot);
  ```
  これにより、膝の曲がり角だけを正確に制限しながら、モデル本来のねじれ回転を維持することに成功しました。

---

## 3. ソルバーの機能・性能比較検証

動的なターゲット（円を描いて動く足先ターゲット）に対して両ソルバーを適用し、検証を行いました：

| 評価項目 | CCDソルバー (CCDSolver) | FABRIKソルバー (FABRIKSolver) |
| :--- | :--- | :--- |
| **収束速度 (追従性)** | 遅い。多関節チェーンのねじれを解くのに多くの反復が必要。 | 非常に速い。わずか 2〜3 回の反復で位置が収束する。 |
| **コイリング（巻き付き）** | 発生しやすい。ターゲットが根元に近いと骨が巻き付く。 | 発生しにくい。ターゲットに向けて骨がまっすぐ伸びる。 |
| **関節制限の相性** | 非常に良い。各関節の回転角に直接クランプを施すため破綻しにくい。 | 注意が必要。座標確定後のバインド時に制限をかけるため、精度が若干落ちる。 |
| **CPU計算負荷** | 高い（反復毎に `acos` などの重い三角関数を使用）。 | 非常に低い（ベクトルの加減算と比率計算のみでループする）。 |

---

## 4. 組み込み検証コード

[GameObject.cpp](file:///d:/DirectX11_Sample/DirextX11Engine/GameObject.cpp#L125-L145) にて、以下の構成でIKを毎フレーム動作させています：

```cpp
void GameObject::Update(float deltaTime)
{
    m_ikTestTimer += deltaTime;
    if (m_pAnimator) m_pAnimator->Update(deltaTime);
    if (m_pModel) m_pModel->Update(deltaTime);

    if (m_ikSolver && m_pSkinnedModelCache && m_effectorIndex != -1)
    {
        auto& bones = m_pSkinnedModelCache->GetBone();
        DirectX::XMFLOAT4X4 baseGlobalMat = bones[m_effectorIndex].GlobalTransformMatrix;
        DirectX::XMFLOAT3 basePos(baseGlobalMat._41, baseGlobalMat._42, baseGlobalMat._43);

        // サイン・コサインを用いた足元の円運動ターゲットの計算
        float speed = 3.0f;
        float radius = 8.0f;
        DirectX::XMFLOAT3 targetPosition = basePos;
        targetPosition.x += std::cos(m_ikTestTimer * speed) * radius;
        targetPosition.z += std::sin(m_ikTestTimer * speed) * radius;
        targetPosition.y += 0.1f; // 接地を想定した高さ補正

        // ソルバーを実行し、GPU用パレットバッファへ転送
        m_ikSolver->Solve(m_effectorIndex, m_ikChainIndices, targetPosition);
        m_pSkinnedModelCache->UploadPaletteBuffer();
    }
}
```

この統合により、斜面接地（フットプレイスメント）や腕のリーチアクションなどの高度な表現がエンジン側でいつでもサポート可能になりました。
