#include "GameObject.h"

namespace Engine
{
    GameObject::GameObject() {}
    GameObject::~GameObject() {}

    void GameObject::SetModel(const std::shared_ptr<IModel>& pModel)
    {
        m_pModel = pModel;
    }
    void GameObject::SetAnimator(const std::shared_ptr<Animator>& pAnimator)
    {
        m_pAnimator = pAnimator;
        m_pAnimator->Play("Idle");
    }
    void GameObject::InitIK()
    {
        // 1. モデルが SkinnedModel であるか判定してキャッシュ
        auto skinnedModel = std::dynamic_pointer_cast<SkinnedModel>(m_pModel);
        if (!skinnedModel) return;
        m_pSkinnedModelCache = skinnedModel.get();

        // 2. ソルバーの生成 (最大反復回数: 10, 許容誤差: 0.001)
        //m_ikSolver = std::make_unique<CCDSolver>(10, 0.001f, skinnedModel);
        m_ikSolver = std::make_unique<FABRIKSolver>(10, 0.001f, skinnedModel);
        
        // 3. ボーン名からインデックスを解決してチェーンを構築
        // ※ GetBoneIndex() は SkinnedModel に追加したボーン名逆引き関数を想定
        m_effectorIndex = m_pSkinnedModelCache->GetBoneIndex("mixamorig:LeftFoot");
        m_ikChainIndices.clear();
        m_ikChainIndices.push_back(m_pSkinnedModelCache->GetBoneIndex("mixamorig:LeftUpLeg"));
        m_ikChainIndices.push_back(m_pSkinnedModelCache->GetBoneIndex("mixamorig:LeftLeg"));
        m_ikChainIndices.push_back(m_pSkinnedModelCache->GetBoneIndex("mixamorig:LeftFoot"));

        // GameObject.cpp の InitIK() の末尾に追加する例
        int leftLegIdx = m_pSkinnedModelCache->GetBoneIndex("mixamorig:LeftLeg");
        // 膝関節に対し、ローカルX軸 (1.0, 0.0, 0.0) まわりで 0度 〜 120度 の曲がり角制限を設定
        m_ikSolver->SetConstraint(
            leftLegIdx,
            DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            120.0f
        );
    }

    void GameObject::AddChild(const std::shared_ptr<GameObject>& pChild)
    {
        // 子オブジェクトの Transform に対し、自身の Transform のメモリアドレスを親として設定する
        pChild->GetTransform().SetParent(&m_transform);
        m_children.push_back(pChild);
    }

    DirectX::XMFLOAT3 GameObject::GetWorldPosition()
    {
        // ローカルの中心座標を取得
		auto localSphere = m_pModel->GetLocalBoundingSphere().Center;
		DirectX::XMVECTOR localCenter = DirectX::XMLoadFloat3(&localSphere);

        // ワールド行列を書けて、現在のゲーム世界での位置を計算する
		auto worldMatrix = m_transform.GetWorldMatrix();
		DirectX::XMVECTOR worldCenter = DirectX::XMVector3TransformCoord(localCenter, worldMatrix);

        DirectX::XMFLOAT3 result;
		DirectX::XMStoreFloat3(&result, worldCenter);

        return result;
	}

    float GameObject::GetBoundingSphereRadiusScaled()
    {
        float localRadius = m_pModel->GetLocalBoundingSphere().Radius;

		// ワールド行列からスケーリング成分を分解して、最大のスケール値を取得する
        DirectX::XMVECTOR scale, rot, trans;
		DirectX::XMMatrixDecompose(&scale, &rot, &trans, m_transform.GetWorldMatrix());

        // スケールのうち、一番大きい値を取得する
		DirectX::XMFLOAT3 fscale;
		DirectX::XMStoreFloat3(&fscale, scale);

        float maxScale = max(fscale.x, max(fscale.y, fscale.z));

        // ローカルの半径に最大スケール値を乗算して、ワールド空間での半径を計算する
        return localRadius * maxScale;
    }

    void GameObject::Draw(Graphics* pGfx, DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
    {
        // --- TODO 3: 定数バッファの更新と自身の描画 ---
        /* m_pModel が有効である場合、
           1. 自身の Transform から GetWorldMatrix() を呼び出し、ワールド行列を取得せよ。
           2. pGfx->Update() を呼び出し、ワールド、ビュー、プロジェクション行列をVRAMの定数バッファへ転送せよ。
           3. m_pModel->Draw() を呼び出し、レンダリングコマンドをディスパッチせよ。
        */
        if (!m_pModel)
        {
            return;
        }

		DirectX::XMMATRIX world = m_transform.GetWorldMatrix();
		pGfx->Update(world, view, proj);
		m_pModel->Draw();

        // --- TODO 4: 子オブジェクトの再帰的描画 ---
        /* m_children 配列を反復処理し、各子オブジェクトの Draw メソッドを再帰的に呼び出せ。
           引数にはホスト層から受け取った pGfx, view, proj をそのまま伝播させること。
        */
        for (const auto child : m_children)
        {
            if (!child)
            {
                continue;
			}

			child->Draw(pGfx, view, proj);
        }
    }

    void GameObject::Update(float deltaTime)
    {
        // タイマーの更新
        m_ikTestTimer += deltaTime;
        // A. アニメーションキーフレームの補間 (LocalTransformMatrix の更新)
        if (m_pAnimator) m_pAnimator->Update(deltaTime);
        // B. モデル全体の初期FK更新 (初期の GlobalTransformMatrix の確定)
        if (m_pModel) m_pModel->Update(deltaTime);
        // C. IKソルバーの実行
        if (m_ikSolver && m_pSkinnedModelCache && m_effectorIndex != -1)
        {
            // モデルから直接ボーン配列の参照を取得（値コピーなし）
            auto& bones = m_pSkinnedModelCache->GetBone();
            // アニメーションFK適用後のエフェクタ（足首）の初期グローバル位置をベースとする
            DirectX::XMFLOAT4X4 baseGlobalMat = bones[m_effectorIndex].GlobalTransformMatrix;
            DirectX::XMFLOAT3 basePos(baseGlobalMat._41, baseGlobalMat._42, baseGlobalMat._43);
            // サイン・コサイン波による円運動のターゲット座標計算
            float speed = 3.0f;
            float radius = 8.0f;
            DirectX::XMFLOAT3 targetPosition = basePos;
            targetPosition.x += std::cos(m_ikTestTimer * speed) * radius;
            targetPosition.z += std::sin(m_ikTestTimer * speed) * radius;
            targetPosition.y += 0.1f; // 少し持ち上げる
            // Solveを呼び出し、モデルの bones 配列を直接書き換える
            m_ikSolver->Solve(m_effectorIndex, m_ikChainIndices, targetPosition);
            // D. 最終確定したポーズデータを行列パレットバッファに再書き込みしてGPUへ転送
            m_pSkinnedModelCache->UploadPaletteBuffer();
        }
    }
}