#include "Material.h"

namespace Engine
{
    Material::Material() : m_BaseColor(1.0f, 1.0f, 1.0f, 1.0f), m_Metallic(1.0f), m_Roughness(0.15f) {}
    Material::~Material() {}

    void Material::SetShader(std::weak_ptr<Shader> pShader)
    {
        m_pShader = pShader;
    }

    void Material::SetTexture(const std::shared_ptr<Texture>& pTexture)
    {
        m_pTexture = pTexture;
    }

    void Material::SetBaseColor(float r, float g, float b, float a) { m_BaseColor = DirectX::XMFLOAT4(r, g, b, a); }
    void Material::SetMetallic(float metallic) { m_Metallic = metallic; }
    void Material::SetRoughness(float roughness) { m_Roughness = roughness; }

    HRESULT Material::CreateBuffer(ID3D11Device* device)
    {
        // --- TODO 2: マテリアル定数バッファの生成 ---
        /*
            b1レジスタにバインドするための専用のID3D11Bufferを生成してください。
            1. D3D11_BUFFER_DESC desc = {};
            2. Usage に D3D11_USAGE_DYNAMIC を指定。
            3. ByteWidth に sizeof(MaterialConstantBufferData) を指定。
            4. BindFlags に D3D11_BIND_CONSTANT_BUFFER を指定。
            5. CPUAccessFlags に D3D11_CPU_ACCESS_WRITE を指定。
            6. device->CreateBuffer を呼び出して m_pMaterialCB に格納する。
        */

		D3D11_BUFFER_DESC desc = {};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(MaterialConstantBufferData);
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = device->CreateBuffer(&desc, nullptr, &m_pMaterialCB);
        
        return hr;
    }

    void Material::Bind(Graphics* pGfx)
    {
        std::shared_ptr<Shader> shader = m_pShader.lock();
        if(!shader) return;
        shader->Render(pGfx->GetContext());

        if (m_pTexture)
        {
            ID3D11ShaderResourceView* srv = m_pTexture->GetSRV();
            pGfx->GetContext()->PSSetShaderResources(0, 1, &srv);
        }

        // --- TODO 3: マテリアルデータのVRAM転送とバインド ---
        /* 
           m_pMaterialCB が有効な場合、pGfx->GetContext()->Map を呼び出して
           MaterialConstantBufferData 構造体に m_BaseColor, m_Metallic, m_Roughness 
           の値をコピーし、GPUへ転送してください。
           その後、Unmap を実行し、PSSetConstantBuffers(1, 1, m_pMaterialCB.GetAddressOf())
           を呼び出して b1 レジスタにバッファをバインドします。
        */

        ID3D11DeviceContext* p_context = pGfx->GetContext();

        D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = p_context->Map(m_pMaterialCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
        {
            return;
        }

		MaterialConstantBufferData* data = (MaterialConstantBufferData*)mapped.pData;
		data->BaseColor = m_BaseColor;
		data->Metallic = m_Metallic;
		data->Roughness = m_Roughness;

		p_context->Unmap(m_pMaterialCB.Get(), 0);
        p_context->PSSetConstantBuffers(1, 1, m_pMaterialCB.GetAddressOf());
    }
}
