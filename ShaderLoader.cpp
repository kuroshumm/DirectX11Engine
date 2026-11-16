#include "ShaderLoader.h"

namespace Engine
{
    namespace
    {
        // 共通のシェーダー初期化およびコンパイル処理
        std::shared_ptr<Shader> CompileShader(
            ID3D11Device* pDevice,
            const wchar_t* vsPath,
            const wchar_t* psPath)
        {
            if (!pDevice || !vsPath || !psPath)
            {
                return nullptr;
            }

            auto pShader = std::make_shared<Shader>();
            if (FAILED(pShader->Initialize(pDevice)))
            {
                return nullptr;
            }

            // 頂点シェーダーのコンパイル
            if (FAILED(pShader->CompileVertexShader(vsPath, "VSMain")))
            {
                return nullptr;
            }

            // ピクセルシェーダーのコンパイル
            if (FAILED(pShader->CompilePixelShader(psPath, "PSMain")))
            {
                return nullptr;
            }

            return pShader;
        }
    }

    std::shared_ptr<Shader> ShaderLoader::LoadSkinned(
        ID3D11Device* pDevice,
        const wchar_t* vsPath,
        const wchar_t* psPath)
    {
        auto pShader = CompileShader(pDevice, vsPath, psPath);
        if (!pShader)
        {
            return nullptr;
        }

        // スキニング用インプットレイアウトの作成
        if (FAILED(pShader->CreateSkinnrdInputLayout()))
        {
            return nullptr;
        }

        return pShader;
    }

    std::shared_ptr<Shader> ShaderLoader::LoadStatic(
        ID3D11Device* pDevice,
        const wchar_t* vsPath,
        const wchar_t* psPath)
    {
        auto pShader = CompileShader(pDevice, vsPath, psPath);
        if (!pShader)
        {
            return nullptr;
        }

        // 静的プリミティブ用インプットレイアウトの作成
        if (FAILED(pShader->CreateStaticInputLayout()))
        {
            return nullptr;
        }

        return pShader;
    }
}

