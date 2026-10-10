#pragma once

#include <memory>
#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
    // ★ 前方宣言により、ヘッダーの依存関係を最小化！
    class FABRIKSolver;
    class CCDSolver;

    class IKSystem
    {
    public:
        IKSystem();
        ~IKSystem(); // unique_ptr の完全な型デストラクタのため .cpp で実装

        void Update(entt::registry& registry, float dt);

    private:
        // ★ ポインタメンバ変数として保持！
        std::unique_ptr<FABRIKSolver> m_pFabrikSolver;
        std::unique_ptr<CCDSolver>    m_pCcdSolver;
    };
}