#pragma once
#include <vector>
#include <wrl/client.h>
#include <d3d11.h>

// ジオメトリの描画最小単位
struct SubMesh
{
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    UINT indexCount = 0;
    UINT vertexStride = 0;
    int materialIndex = 0;

    // 共通静的ファクトリ関数: 任意の頂点型から SubMesh (GPUバッファ) を生成
    template<typename VertexType>
    static SubMesh Create(
        ID3D11Device* pDevice,
        const std::vector<VertexType>& vertices,
        const std::vector<WORD>& indices,
        int materialIndex = 0)
    {
        // ガード節
        if (!pDevice || vertices.empty() || indices.empty())
        {
            throw std::invalid_argument("Invalid arguments for SubMesh::Create");
        }
        SubMesh subMesh;
        subMesh.materialIndex = materialIndex;
        subMesh.indexCount = static_cast<UINT>(indices.size());
        subMesh.vertexStride = sizeof(VertexType);
        // ① 頂点バッファ (VertexBuffer) の生成
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = sizeof(VertexType) * static_cast<UINT>(vertices.size());
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA vinit = { vertices.data() };
        HRESULT hr = pDevice->CreateBuffer(&vbd, &vinit, &subMesh.vertexBuffer);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create vertex buffer.");
        }
        // ② インデックスバッファ (IndexBuffer) の生成
        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = sizeof(WORD) * static_cast<UINT>(indices.size());
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA iinit = { indices.data() };
        hr = pDevice->CreateBuffer(&ibd, &iinit, &subMesh.indexBuffer);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create index buffer.");
        }
        return subMesh;
    }
};
