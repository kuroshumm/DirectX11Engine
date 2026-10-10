#include "HierarchyTransformSystem.h"

namespace Engine
{
	void HierarchyTransformSystem::Update(entt::registry& registry)
	{
		auto view = registry.view<TransformComponent>();

		for (auto [entity, transformComp] : view.each())
		{
			HierarchyComponent* hierarchyComp = registry.try_get<HierarchyComponent>(entity);

			// ルートノードのみ処理を開始
			if (!hierarchyComp || hierarchyComp->parent == entt::null)
			{
				DirectX::XMMATRIX worldMat;
				if (transformComp.isDirty)
				{
					worldMat = ComputeLocalMatrix(transformComp);
					DirectX::XMStoreFloat4x4(&transformComp.worldMatrix, worldMat);
				}
				else
				{
					// 静止している場合は既存のワールド行列を再利用（無駄な計算を回避）
					worldMat = DirectX::XMLoadFloat4x4(&transformComp.worldMatrix);
				}

				// 子孫ノードへ再帰伝播（Dirtyフラグを引き継ぐ）
				if (hierarchyComp && hierarchyComp->firstChild != entt::null)
				{
					UpdateSubtree(registry, hierarchyComp->firstChild, worldMat, transformComp.isDirty);
				}

				transformComp.isDirty = false;
			}
		}
	}

	void HierarchyTransformSystem::UpdateSubtree(
		entt::registry& registry,
		entt::entity current,
		const DirectX::XMMATRIX& parentWorldMatrix,
		bool parentIsDirty)
	{
		entt::entity curr = current;
		while (curr != entt::null && registry.valid(curr))
		{
			auto& childTransform = registry.get<TransformComponent>(curr);
			auto& childHierarchy = registry.get<HierarchyComponent>(curr);

			bool needUpdate = childTransform.isDirty || parentIsDirty;
			DirectX::XMMATRIX childWorldMat;

			if (needUpdate)
			{
				DirectX::XMMATRIX childLocalMat = ComputeLocalMatrix(childTransform);
				// 子のワールド行列 = 子のローカル行列 * 親のワールド行列
				childWorldMat = DirectX::XMMatrixMultiply(childLocalMat, parentWorldMatrix);
				DirectX::XMStoreFloat4x4(&childTransform.worldMatrix, childWorldMat);
			}
			else
			{
				childWorldMat = DirectX::XMLoadFloat4x4(&childTransform.worldMatrix);
			}

			// 子ノードへ再帰
			if (childHierarchy.firstChild != entt::null)
			{
				UpdateSubtree(
					registry,
					childHierarchy.firstChild,
					childWorldMat,
					needUpdate
				);
			}

			childTransform.isDirty = false;

			curr = childHierarchy.nextSibling;
		}
	}

	DirectX::XMMATRIX HierarchyTransformSystem::ComputeLocalMatrix(const TransformComponent& transform)
	{
		DirectX::XMVECTOR s = DirectX::XMLoadFloat3(&transform.scale);
		DirectX::XMVECTOR r = DirectX::XMLoadFloat4(&transform.rotation);
		DirectX::XMVECTOR t = DirectX::XMLoadFloat3(&transform.position);

		return DirectX::XMMatrixAffineTransformation(s, DirectX::XMVectorZero(), r, t);
	}

}