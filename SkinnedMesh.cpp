#include "SkinnedMesh.h"

namespace Engine
{
    SkinnedMesh::SkinnedMesh(Graphics* pGfx) : m_pGfx(pGfx), m_indexCount(0)
    {

    }
    SkinnedMesh::~SkinnedMesh() {}

    HRESULT SkinnedMesh::Create(const std::vector<SkinnedVertex>& vertices, const std::vector<WORD>& indices)
    {
        m_indexCount = (UINT)indices.size();

        // 頂点バッファの生成
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = sizeof(SkinnedVertex) * (UINT)vertices.size();
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage     = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA vinit = { vertices.data() };
        HRESULT hr = m_pGfx->GetDevice()->CreateBuffer(&vbd, &vinit, &m_pVertexBuffer);
        if (FAILED(hr))
        {
            return hr;
        }

        // インデックスバッファの生成
        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = sizeof(WORD) * (UINT)indices.size();
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage     = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA iinit = { indices.data() };
        hr = m_pGfx->GetDevice()->CreateBuffer(&ibd, &iinit, &m_pIndexBuffer);

        return hr;
    }

    void SkinnedMesh::Draw()
    {
        UINT stride = sizeof(SkinnedVertex);
        UINT offset = 0;

        ID3D11DeviceContext* context = m_pGfx->GetContext();
        context->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, offset);
        context->DrawIndexed(m_indexCount, 0, 0);
    }
}