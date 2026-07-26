#include "Mesh.h"

namespace Engine
{
    Mesh::Mesh(Graphics* pGfx) : m_pGfx(pGfx), m_indexCount(0) 
    {
        
    }
    Mesh::~Mesh() {}

    HRESULT Mesh::Create(const std::vector<Vertex>& vertices, const std::vector<WORD>& indices)
    {
        m_indexCount = (UINT)indices.size();

        // --- 頂点バッファ (VertexBuffer) の生成 ---
        // 生成後は変更されないため D3D11_USAGE_IMMUTABLE を使用し、PCIe経由の転送を最小化
        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = sizeof(Vertex) * (UINT)vertices.size();
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA vinit = { vertices.data() };
        HRESULT hr = m_pGfx->GetDevice()->CreateBuffer(&vbd, &vinit, &m_pVertexBuffer);
        if (FAILED(hr)) return hr;

        // --- インデックスバッファ (IndexBuffer) の生成 ---
        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = sizeof(WORD) * (UINT)indices.size();
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        D3D11_SUBRESOURCE_DATA iinit = { indices.data() };
        return m_pGfx->GetDevice()->CreateBuffer(&ibd, &iinit, &m_pIndexBuffer);
    }

    void Mesh::Draw()
    {
        // --- インプットアセンブラ (IA) ステージへのバッファバインド ---
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_pGfx->GetContext()->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
        m_pGfx->GetContext()->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        // インデックスを使用したプリミティブの描画
        m_pGfx->GetContext()->DrawIndexed(m_indexCount, 0, 0);
    }
}