#include "TransformSystem.h"
#include "Components.h"

namespace Engine
{
    void TransformSystem::Update(entt::registry& registry)
    {
        auto entities = registry.view<TransformComponent>(entt::exclude<HierarchyComponent>);

        for (auto [entity, transform] : entities.each())
        {
            // スケール行列
            DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(
                transform.scale.x,
                transform.scale.y,
                transform.scale.z
            );
            // 回転行列
            DirectX::XMVECTOR rotQuat = DirectX::XMLoadFloat4(&transform.rotation);
            DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotQuat);
            // 位置行列
            DirectX::XMMATRIX posMat = DirectX::XMMatrixTranslation(
                transform.position.x,
                transform.position.y,
                transform.position.z
            );

            // S x R x Tによる合成
            DirectX::XMMATRIX worldMat = scaleMat * rotMat * posMat;

            // 計算結果をコンポーネントへ保存
            DirectX::XMStoreFloat4x4(&transform.worldMatrix, worldMat);
        }
    }

}
