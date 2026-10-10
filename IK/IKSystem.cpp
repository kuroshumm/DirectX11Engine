#include "IKSystem.h"
#include "FABRIKSolver.h"
#include "CCDSolver.h"

namespace Engine
{
    IKSystem::IKSystem()
        : m_pFabrikSolver(std::make_unique<FABRIKSolver>(10, 0.001f))
        , m_pCcdSolver(std::make_unique<CCDSolver>(10, 0.001f))
    {
    }

    IKSystem::~IKSystem() = default;

    void IKSystem::Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<SkeletonComponent, IKComponent, AnimationUpdateTag>();

        for (auto [entity, skeltonComp, ikComp] : view.each())
        {
            if (skeltonComp.bones.empty() || !ikComp.isEnabled || ikComp.effectorBoneIndex == -1)
            {
                continue;
            }

            std::vector<Bone>& bones = skeltonComp.bones;
            switch (ikComp.solverType)
            {
            case IKSolverType::CCD:
                if (m_pCcdSolver) m_pCcdSolver->Solve(bones, ikComp, ikComp.targetPosition);
                break;
            case IKSolverType::FABRIK:
                if (m_pFabrikSolver) m_pFabrikSolver->Solve(bones, ikComp, ikComp.targetPosition);
                break;
            }
        }
    }
}