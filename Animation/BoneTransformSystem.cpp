#include "BoneTransformSystem.h"

namespace Engine
{
    void BoneTransformSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<SkeletonComponent, AnimationUpdateTag>();

        for (auto [entity, skeltonComp] : view.each())
        {
            if (skeltonComp.bones.empty() || skeltonComp.rootBoneIndex == -1)
            {
                continue;
            }

            UpdateBoneTransform(skeltonComp.bones, skeltonComp.rootBoneIndex, DirectX::XMMatrixIdentity());
        }
    }

    void BoneTransformSystem::UpdateBoneTransform(
        std::vector<Bone>& bones,
        int boneIndex,
        DirectX::XMMATRIX parentGlocbalTransform
    )
    {
        Bone& bone = bones[boneIndex];

        DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone.LocalTransformMatrix);
        DirectX::XMMATRIX global = DirectX::XMMatrixMultiply(local, parentGlocbalTransform);
        DirectX::XMStoreFloat4x4(&bone.GlobalTransformMatrix, global);

        // 子供たちへ再帰的にFKを伝搬させる
        for (int childIndex : bone.childIndices)
        {
            UpdateBoneTransform(bones, childIndex, global);
        }
    }
}
