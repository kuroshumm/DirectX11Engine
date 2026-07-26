#include "Shader.h"

namespace Engine
{
    Shader::Shader()
    {
	}

    Shader::~Shader()
    {
    }

    HRESULT Shader::Initialize(ID3D11Device* pDeveice)
    {
        m_pDevice = pDeveice;
		return S_OK;
    }

    void Shader::Render(ID3D11DeviceContext* pContext)
    {
        pContext->IASetInputLayout(m_pLayout.Get());
        pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
        pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
	}

    HRESULT Shader::CompilePixelShader(const wchar_t* filePath, const char* entryPoint)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            filePath,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            "ps_5_0",
            0,
            0,
            &m_psBlob,
            &errorBlob
        );
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return hr;
        }
        m_pDevice->CreatePixelShader(m_psBlob->GetBufferPointer(), m_psBlob->GetBufferSize(), nullptr, &m_pPS);
    }

    HRESULT Shader::CompileVertexShader(const wchar_t* filePath, const char* entryPoint)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            filePath,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint,
            "vs_5_0",
            0,
            0,
            &m_vsBlob,
            &errorBlob
        );
        if (FAILED(hr))
        {
            if (errorBlob) OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            return hr;
        }
        m_pDevice->CreateVertexShader(m_vsBlob->GetBufferPointer(), m_vsBlob->GetBufferSize(), nullptr, &m_pVS);
    }

    HRESULT Shader::CreateInputLayout()
    {
        // --- インプットレイアウトの定義 (修正箇所) ---
        // 第9回 HLSL の VS_INPUT シグネチャと厳密に一致させる必要がある
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 12 bytes
            { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 12 bytes
            { "TANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 12 bytes (第9回追加)
            { "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 16 bytes
            { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 8 bytes
            { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 16 bytes
            { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 76, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // 16 bytes
        };

        // 頂点シェーダーのバイナリコードを取得して入力レイアウトを生成
        HRESULT hr = m_pDevice->CreateInputLayout(
            layout,
            ARRAYSIZE(layout),
            m_vsBlob->GetBufferPointer(),
            m_vsBlob->GetBufferSize(),
            &m_pLayout
        );

        return hr;
    }
}