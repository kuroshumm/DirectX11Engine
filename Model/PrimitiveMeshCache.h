#pragma once
#include <memory>

#include "Common.h"
#include "Components.h"
#include "GeometryGenerator.h"

namespace Engine
{
	class PrimitiveMeshCache
	{
	public:
		static std::shared_ptr<ModelAsset> GetOrCreate(ID3D11Device* pDevice, PrimitiveType type);

	private:
		static std::shared_ptr<ModelAsset> CreateModelAsset(ID3D11Device* pDevice, const MeshData& meshData);

	private:
		static constexpr size_t TYPE_COUNT = 4;
		inline static std::array<std::shared_ptr<ModelAsset>, TYPE_COUNT> m_cache{};
	};
}