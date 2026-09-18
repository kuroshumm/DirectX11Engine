#include "PrimitiveMeshCache.h"
#include "GeometryGenerator.h"

namespace Engine
{
    std::shared_ptr<ModelAsset> PrimitiveMeshCache::GetOrCreate(ID3D11Device* pDevice, PrimitiveType type)
    {
        size_t typeIndex = static_cast<size_t>(type);
        if (typeIndex >= TYPE_COUNT)
        {
            return nullptr;
        }

        if (m_cache[typeIndex] != nullptr)
        {
            return m_cache[typeIndex];
        }

        MeshData meshData;
        switch (type)
        {
        case PrimitiveType::Plane:
            // 地面用プレーン (50m x 50m, 分割数 50x50, UVタイリング 50x50)
            meshData = GeometryGenerator::CreatePlane(50.0f, 50.0f, 50, 50, 50.0f, 50.0f);
            break;
        case PrimitiveType::Cube:
            // 単位キューブ (1m x 1m x 1m) -> 大きさは TransformComponent.scale で制御
            meshData = GeometryGenerator::CreateCube(1.0f, 1.0f, 1.0f);
            break;
        case PrimitiveType::Sphere:
            // 単位球 (半径 0.5m -> 直径 1m, スライス 20, スタック 20)
            meshData = GeometryGenerator::CreateSphere(0.5f, 20, 20);
            break;
        case PrimitiveType::Cylinder:
            // 単位円柱 (半径 0.5m, 高さ 1m, スライス 20, スタック 1)
            meshData = GeometryGenerator::CreateCylinder(0.5f, 0.5f, 1.0f, 20, 1);
            break;
        }
        // ③ GPUバッファと境界球を構築し、キャッシュに保存して返却
        m_cache[typeIndex] = CreateModelAsset(pDevice, meshData);
        return m_cache[typeIndex];
    }

    std::shared_ptr<ModelAsset> PrimitiveMeshCache::CreateModelAsset(
        ID3D11Device* pDevice,
        const MeshData& meshData
    )
    {
        std::shared_ptr<ModelAsset> pModelAsset = std::make_shared<ModelAsset>();

        auto vertices = meshData.Vertices;
        auto indices = meshData.Indices;
        SubMesh subMesh = SubMesh::Create<Vertex>(pDevice, vertices, indices);

        DirectX::BoundingSphere::CreateFromPoints(
            pModelAsset->totalLocalBounds,
            vertices.size(),
            &vertices[0].Pos,
            sizeof(Vertex)
        );

        pModelAsset->subMeshs.push_back(std::move(subMesh));

        return pModelAsset;
    }
}