#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

#include "Common.h"
#include "Shader.h"

namespace Engine
{
    class Graphics
    {
    public:
        Graphics();
        ~Graphics();

        HRESULT Initialize(HWND hWnd, int width, int height);
        void BeginScene(float r, float g, float b, float a);
        void EndScene();
        void SetRender();
		void SetTextureAndSampler();

        void Update(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj);

        ID3D11Device* GetDevice() { return m_pDevice.Get(); }
        ID3D11DeviceContext* GetContext() { return m_pContext.Get(); }

		HRESULT InitShaders();
        HRESULT InitConstantBuffer();
        HRESULT InitPipelineStates();

        // パイプラインステート管理
        void SetRasterizerState(ID3D11RasterizerState* pState);
        void SetDepthStencilState(ID3D11DepthStencilState* pState);

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_pContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain>         m_pSwapChain;
        Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRTV;

        Microsoft::WRL::ComPtr<ID3D11RasterizerState>  m_pRS;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_pDSS;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_pDSV;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_pDepthStencil;

        Microsoft::WRL::ComPtr<ID3D11Texture2D>            m_pTexture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   m_pSRV;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>         m_pSampler;
    };
}