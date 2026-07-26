#pragma once
#include <memory>
#include <vector>
#include <DirectXMath.h>
#include "Graphics.h"
#include "IModel.h"
#include "Transform.h"
#include "Animator.h"
#include "CCDSolver.h"
#include "FABRIKSolver.h"

namespace Engine
{
    // 空間座標と描画リソースを統合管理する論理エンティティクラス
    class GameObject
    {
    public:
        GameObject();
        ~GameObject();

        void SetModel(const std::shared_ptr<IModel>& pModel);
        void SetAnimator(const std::shared_ptr<Animator>& pAnimator);
        void InitIK();

        // 子オブジェクトをシーングラフに追加し、親子の依存関係を構築する
        void AddChild(const std::shared_ptr<GameObject>& pChild);

        Transform& GetTransform() { return m_transform; }

        // ワールド座標の中心座標を取得する
		DirectX::XMFLOAT3 GetWorldPosition();
		// 拡大縮小（スケール）が適用された後の半径を取得する
		float GetBoundingSphereRadiusScaled();


        // オブジェクトおよび子階層全体の描画コマンドを発行する
        void Draw(Graphics* pGfx, DirectX::XMMATRIX view, DirectX::XMMATRIX proj);
        void Update(float deltaTime);

    private:
        Transform m_transform;
        std::shared_ptr<IModel> m_pModel;
        std::shared_ptr<Animator> m_pAnimator;
        std::unique_ptr<FABRIKSolver> m_ikSolver;

        // 子ノードに対する排他的所有権 (コンポジション)
        std::vector<std::shared_ptr<GameObject>> m_children;


        SkinnedModel* m_pSkinnedModelCache = nullptr; // 高速化用の生ポインタキャッシュ
        std::vector<int>           m_ikChainIndices;
        int                        m_effectorIndex = -1;
        float                      m_ikTestTimer = 0.0f; // サイン波動作用のタイマー
    };
}