#include "BonePaletteUploadSystem.h"
#include <algorithm>
#include <stdexcept>

namespace Engine
{
	void BonePaletteUploadSystem::Update(entt::registry& registry, ID3D11DeviceContext& context)
	{
		auto view = registry.view<SkeletonComponent, SkinningPaletteComponent, AnimationUpdateTag>();

		for (auto [entity, skeltonComp, skinningPaletteComp] : view.each())
		{
			if (skeltonComp.bones.empty() || skeltonComp.rootBoneIndex == -1)
			{
				continue;
			}
			if (!skinningPaletteComp.pPaletteBuffer)
			{
				continue;
			}

			// 全ての計算が終わった行列を使って最終スキニング行列を作りパレットに書き込む
			// 計算したスキニング行列をGPUに渡す
			UploadPaletteBuffer(context, skeltonComp, skinningPaletteComp);
		}
	}

	void BonePaletteUploadSystem::UploadPaletteBuffer(
		ID3D11DeviceContext& context,
		SkeletonComponent& skeltonComp,
		SkinningPaletteComponent& skinningPaletteComp
	)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(context.Map(
			skinningPaletteComp.pPaletteBuffer.Get(),
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mapped))
		)
		{
			PaletteBufferData* data = (PaletteBufferData*)mapped.pData;

			int maxBoneNum = (std::min)(static_cast<int>(skeltonComp.bones.size()), MAX_BONES);
			for (int boneIndex = 0; boneIndex < maxBoneNum; ++boneIndex)
			{
				Bone& bone = skeltonComp.bones[boneIndex];

				// 3. 【グローバル行列の算出（FK）】
				// 自身のローカルアニメーション × 親の絶対座標 ＝ 自身の絶対座標（足元基準の ModelTransformMatrix）
				DirectX::XMMATRIX globalBoneTransform = DirectX::XMLoadFloat4x4(&bone.GlobalTransformMatrix);

				// 4. 【最終スキニング行列の算出】
				// 頂点をボーンのローカル空間（関節の付け根）に引き戻すための初期逆行列
				DirectX::XMMATRIX inverseBindMat = DirectX::XMLoadFloat4x4(&bone.InverseBindMatrix);
				// 引き戻し（InverseBind）× 新しい絶対座標（globalBoneTransform）
				// これにより「頂点を関節へ引き戻す -> 回転させる -> モデル空間へ戻す」という一連の変形行列が完成する
				DirectX::XMMATRIX skinningMat = DirectX::XMMatrixMultiply(inverseBindMat, globalBoneTransform);

				// 5. 【パレットへの格納】
				// HLSL（頂点シェーダー）側は列優先（Column-Major）を期待するため、転置（Transpose）して定数バッファ用配列へ格納する
				data->Bones[boneIndex] = DirectX::XMMatrixTranspose(skinningMat);
			}

			context.Unmap(skinningPaletteComp.pPaletteBuffer.Get(), 0);
		}
	}
}