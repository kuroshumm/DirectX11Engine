#include "GeometryGenerator.h"

namespace Engine
{
	MeshData GeometryGenerator::CreateCube(float width, float height, float depth)
	{
		// 1. ガード節: 不正な寸法を排除
		if (width <= 0.0f || height <= 0.0f || depth <= 0.0f)
		{
			throw std::invalid_argument("Box dimensions must be greater than zero.");
		}

		MeshData cubeMeshData;

        float w2 = 0.5f * width;
        float h2 = 0.5f * height;
        float d2 = 0.5f * depth;

        // 2. 頂点配列の構築 (6面 × 4頂点 = 24頂点)
        Vertex v[24];
        // --- 前面 (Z = -d2 面: 法線 (0, 0, -1), 接線 (1, 0, 0)) ---
        v[0] = Vertex{ {-w2, -h2, -d2}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[1] = Vertex{ {-w2, +h2, -d2}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[2] = Vertex{ {+w2, +h2, -d2}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[3] = Vertex{ {+w2, -h2, -d2}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };
        // --- 背面 (Z = +d2 面: 法線 (0, 0, 1), 接線 (-1, 0, 0)) ---
        v[4] = Vertex{ {+w2, -h2, +d2}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[5] = Vertex{ {+w2, +h2, +d2}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[6] = Vertex{ {-w2, +h2, +d2}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[7] = Vertex{ {-w2, -h2, +d2}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };
        // --- 上面 (Y = +h2 面: 法線 (0, 1, 0), 接線 (1, 0, 0)) ---
        v[8] = Vertex{ {-w2, +h2, -d2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[9] = Vertex{ {-w2, +h2, +d2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[10] = Vertex{ {+w2, +h2, +d2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[11] = Vertex{ {+w2, +h2, -d2}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };
        // --- 下面 (Y = -h2 面: 法線 (0, -1, 0), 接線 (-1, 0, 0)) ---
        v[12] = Vertex{ {-w2, -h2, +d2}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[13] = Vertex{ {-w2, -h2, -d2}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[14] = Vertex{ {+w2, -h2, -d2}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[15] = Vertex{ {+w2, -h2, +d2}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };
        // --- 左面 (X = -w2 面: 法線 (-1, 0, 0), 接線 (0, 0, -1)) ---
        v[16] = Vertex{ {-w2, -h2, +d2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[17] = Vertex{ {-w2, +h2, +d2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[18] = Vertex{ {-w2, +h2, -d2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[19] = Vertex{ {-w2, -h2, -d2}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };
        // --- 右面 (X = +w2 面: 法線 (1, 0, 0), 接線 (0, 0, 1)) ---
        v[20] = Vertex{ {+w2, -h2, -d2}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        v[21] = Vertex{ {+w2, +h2, -d2}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        v[22] = Vertex{ {+w2, +h2, +d2}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f} };
        v[23] = Vertex{ {+w2, -h2, +d2}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f} };

        cubeMeshData.Vertices.assign(&v[0], &v[24]);

        // 3. インデックス配列の構築 (6面 × 2三角形 = 12三角形 = 36インデックス)
        // 各面において、表から見て「時計回り (0->1->2, 0->2->3)」で指定する
        WORD indices[36];
        for (int face = 0; face < 6; ++face)
        {
            WORD baseIndex = static_cast<WORD>(face * 4);
            indices[face * 6 + 0] = baseIndex + 0;
            indices[face * 6 + 1] = baseIndex + 1;
            indices[face * 6 + 2] = baseIndex + 2;
            indices[face * 6 + 3] = baseIndex + 0;
            indices[face * 6 + 4] = baseIndex + 2;
            indices[face * 6 + 5] = baseIndex + 3;
        }
        cubeMeshData.Indices.assign(&indices[0], &indices[36]);

        return cubeMeshData;
	}

    MeshData GeometryGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
    {
        // 1. ガード節: 最低限の分割数（3角形を作れる最低値）を要求
        if (radius <= 0.0f || sliceCount < 3 || stackCount < 2)
        {
            throw std::invalid_argument("Invalid sphere parameters.");
        }

        MeshData meshData;

        // 北極点 (頂点 0)
        Vertex topVertex{ {0.0f, +radius, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f} };
        meshData.Vertices.push_back(topVertex);

        float phiStep = DirectX::XM_PI / stackCount;
        float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;
        // 緯度リングのループ (北極・南極を除く中間リング)
        for (uint32_t i = 1; i <= stackCount - 1; ++i)
        {
            float phi = i * phiStep;
            // 経度のループ
            // ※ テクスチャの継ぎ目 (U=0 と U=1) を分けるため sliceCount + 1 回回す
            for (uint32_t j = 0; j <= sliceCount; ++j)
            {
                float theta = j * thetaStep;
                Vertex v;
                // 球面座標 -> デカルト座標 (X, Y, Z)
                v.Pos.x = radius * sinf(phi) * cosf(theta);
                v.Pos.y = radius * cosf(phi);
                v.Pos.z = radius * sinf(phi) * sinf(theta);
                // 原点中心なので、Pos を正規化すればそのまま法線になる
                DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&v.Pos);
                DirectX::XMStoreFloat3(&v.Normal, DirectX::XMVector3Normalize(p));
                // 接線 (U方向の偏微分: -sin(theta), 0, cos(theta))
                v.Tangent.x = -sinf(theta);
                v.Tangent.y = 0.0f;
                v.Tangent.z = cosf(theta);
                v.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
                v.TexCoord.x = theta / (2.0f * DirectX::XM_PI);
                v.TexCoord.y = phi / DirectX::XM_PI;
                meshData.Vertices.push_back(v);
            }
        }

        // 南極点
        Vertex bottomVertex{ {0.0f, -radius, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} };
        meshData.Vertices.push_back(bottomVertex);
        
         // --- インデックス生成 ---
        // 北極キャップの三角形接続
        for (uint32_t i = 1; i <= sliceCount; ++i)
        {
            meshData.Indices.push_back(0);
            meshData.Indices.push_back(static_cast<WORD>(i + 1));
            meshData.Indices.push_back(static_cast<WORD>(i));
        }
        // 中間リング（Quad を 2 三角形に分割）の接続
        uint32_t baseIndex = 1;
        uint32_t ringVertexCount = sliceCount + 1;
        for (uint32_t i = 0; i < stackCount - 2; ++i)
        {
            for (uint32_t j = 0; j < sliceCount; ++j)
            {
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i * ringVertexCount + j));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i * ringVertexCount + j + 1));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + (i + 1) * ringVertexCount + j));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + (i + 1) * ringVertexCount + j));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i * ringVertexCount + j + 1));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + (i + 1) * ringVertexCount + j + 1));
            }
        }
        // 南極キャップの三角形接続
        uint32_t southPoleIndex = static_cast<uint32_t>(meshData.Vertices.size() - 1);
        baseIndex = southPoleIndex - ringVertexCount;
        for (uint32_t i = 0; i < sliceCount; ++i)
        {
            meshData.Indices.push_back(static_cast<WORD>(southPoleIndex));
            meshData.Indices.push_back(static_cast<WORD>(baseIndex + i));
            meshData.Indices.push_back(static_cast<WORD>(baseIndex + i + 1));
        }
        return meshData;
    }

    MeshData GeometryGenerator::CreatePlane(
        float width, float depth,
        uint32_t m, uint32_t n,
        float uvTilingX, float uvTilingY)
    {
        // 1. ガード節: 寸法および分割数 (最低1グリッド以上)
        if (width <= 0.0f || depth <= 0.0f || m < 1 || n < 1)
        {
            throw std::invalid_argument("Invalid plane parameters.");
        }
        MeshData meshData;
        uint32_t vertexCount = (m + 1) * (n + 1);
        meshData.Vertices.resize(vertexCount);
        float halfWidth = 0.5f * width;
        float halfDepth = 0.5f * depth;
        float dx = width / static_cast<float>(m);
        float dz = depth / static_cast<float>(n);
        float du = uvTilingX / static_cast<float>(m);
        float dv = uvTilingY / static_cast<float>(n);
        // 2. 頂点配列の生成 (手前 -Z から 奥 +Z 方向へ並べる)
        for (uint32_t i = 0; i <= n; ++i)
        {
            float z = halfDepth - static_cast<float>(i) * dz;
            for (uint32_t j = 0; j <= m; ++j)
            {
                float x = -halfWidth + static_cast<float>(j) * dx;
                uint32_t index = i * (m + 1) + j;
                Vertex& v = meshData.Vertices[index];
                v.Pos = { x, 0.0f, z };              // 地面なので Y=0
                v.Normal = { 0.0f, 1.0f, 0.0f };       // 水平面なので真上を向く
                v.Tangent = { 1.0f, 0.0f, 0.0f };      // X軸正方向
                v.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
                v.TexCoord = { j * du, i * dv };
            }
        }
        // 3. インデックス配列の生成 (Quadセルを2枚の三角形に分割)
        // 上から見て時計回り (CW) で接続
        meshData.Indices.reserve(m * n * 6);
        for (uint32_t i = 0; i < n; ++i)
        {
            for (uint32_t j = 0; j < m; ++j)
            {
                // 四角形セルの4隅のインデックス
                WORD topLeft = static_cast<WORD>(i * (m + 1) + j);
                WORD topRight = static_cast<WORD>(topLeft + 1);
                WORD bottomLeft = static_cast<WORD>((i + 1) * (m + 1) + j);
                WORD bottomRight = static_cast<WORD>(bottomLeft + 1);
                // 三角形 1: topLeft -> topRight -> bottomLeft
                meshData.Indices.push_back(topLeft);
                meshData.Indices.push_back(topRight);
                meshData.Indices.push_back(bottomLeft);
                // 三角形 2: bottomLeft -> topRight -> bottomRight
                meshData.Indices.push_back(bottomLeft);
                meshData.Indices.push_back(topRight);
                meshData.Indices.push_back(bottomRight);
            }
        }
        return meshData;
    }

    MeshData GeometryGenerator::CreateCylinder(
        float bottomRadius, float topRadius, float height,
        uint32_t sliceCount, uint32_t stackCount)
    {
        // 1. ガード節
        if (bottomRadius < 0.0f || topRadius < 0.0f || (bottomRadius == 0.0f && topRadius == 0.0f) ||
            height <= 0.0f || sliceCount < 3 || stackCount < 1)
        {
            throw std::invalid_argument("Invalid cylinder parameters.");
        }
        MeshData meshData;
        // --- パートA: 側面 (Side Tube) ---
        float stackHeight = height / static_cast<float>(stackCount);
        float radiusStep = (topRadius - bottomRadius) / static_cast<float>(stackCount);
        uint32_t ringCount = stackCount + 1;
        for (uint32_t i = 0; i < ringCount; ++i)
        {
            float y = -0.5f * height + static_cast<float>(i) * stackHeight;
            float r = bottomRadius + static_cast<float>(i) * radiusStep;
            float dTheta = 2.0f * DirectX::XM_PI / static_cast<float>(sliceCount);
            for (uint32_t j = 0; j <= sliceCount; ++j)
            {
                Vertex v;
                float c = cosf(j * dTheta);
                float s = sinf(j * dTheta);
                v.Pos = { r * c, y, r * s };
                v.TexCoord = {
                    static_cast<float>(j) / sliceCount,
                    1.0f - static_cast<float>(i) / stackCount
                };
                // 接線 (U方向偏微分)
                v.Tangent = { -s, 0.0f, c };
                // 側面の傾き（テーパー）を考慮した法線
                float dr = bottomRadius - topRadius;
                DirectX::XMFLOAT3 bitangent(dr * c, -height, dr * s);
                DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&v.Tangent);
                DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&bitangent);
                DirectX::XMVECTOR N = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(T, B));
                DirectX::XMStoreFloat3(&v.Normal, N);
                v.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
                meshData.Vertices.push_back(v);
            }
        }
        // 側面のインデックス接続
        uint32_t ringVertexCount = sliceCount + 1;
        for (uint32_t i = 0; i < stackCount; ++i)
        {
            for (uint32_t j = 0; j < sliceCount; ++j)
            {
                WORD i0 = static_cast<WORD>(i * ringVertexCount + j);
                WORD i1 = static_cast<WORD>((i + 1) * ringVertexCount + j);
                WORD i2 = static_cast<WORD>((i + 1) * ringVertexCount + j + 1);
                WORD i3 = static_cast<WORD>(i * ringVertexCount + j + 1);
                // 三角形 1: i0 -> i1 -> i2
                meshData.Indices.push_back(i0);
                meshData.Indices.push_back(i1);
                meshData.Indices.push_back(i2);
                // 三角形 2: i0 -> i2 -> i3
                meshData.Indices.push_back(i0);
                meshData.Indices.push_back(i2);
                meshData.Indices.push_back(i3);
            }
        }
        // --- パートB: 上面キャップ (Top Cap) ---
        if (topRadius > 0.0f)
        {
            uint32_t baseIndex = static_cast<uint32_t>(meshData.Vertices.size());
            float y = 0.5f * height;
            float dTheta = 2.0f * DirectX::XM_PI / static_cast<float>(sliceCount);
            // 円周頂点
            for (uint32_t i = 0; i <= sliceCount; ++i)
            {
                float x = topRadius * cosf(i * dTheta);
                float z = topRadius * sinf(i * dTheta);
                // テクスチャ座標は中心 (0.5, 0.5) を基準にマッピング
                float u = x / height + 0.5f;
                float v = z / height + 0.5f;
                meshData.Vertices.push_back(Vertex{
                    { x, y, z }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f }, { u, v }
                    });
            }
            // 中心頂点
            meshData.Vertices.push_back(Vertex{
                { 0.0f, y, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.5f }
                });
            WORD centerIndex = static_cast<WORD>(meshData.Vertices.size() - 1);
            // ファン状に三角形を接続 (時計回り)
            for (uint32_t i = 0; i < sliceCount; ++i)
            {
                meshData.Indices.push_back(centerIndex);
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i + 1));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i));
            }
        }
        // --- パートC: 底面キャップ (Bottom Cap) ---
        if (bottomRadius > 0.0f)
        {
            uint32_t baseIndex = static_cast<uint32_t>(meshData.Vertices.size());
            float y = -0.5f * height;
            float dTheta = 2.0f * DirectX::XM_PI / static_cast<float>(sliceCount);
            for (uint32_t i = 0; i <= sliceCount; ++i)
            {
                float x = bottomRadius * cosf(i * dTheta);
                float z = bottomRadius * sinf(i * dTheta);
                float u = x / height + 0.5f;
                float v = z / height + 0.5f;
                meshData.Vertices.push_back(Vertex{
                    { x, y, z }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f }, { u, v }
                    });
            }
            meshData.Vertices.push_back(Vertex{
                { 0.0f, y, 0.0f }, { 0.0f, -1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
                { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.5f }
                });
            WORD centerIndex = static_cast<WORD>(meshData.Vertices.size() - 1);
            for (uint32_t i = 0; i < sliceCount; ++i)
            {
                meshData.Indices.push_back(centerIndex);
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i));
                meshData.Indices.push_back(static_cast<WORD>(baseIndex + i + 1));
            }
        }
        return meshData;
    }
}