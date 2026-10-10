#include "Graphics.h"

namespace Engine
{
    Graphics::Graphics() {}
    Graphics::~Graphics() {}

    HRESULT Graphics::Initialize(HWND hWnd, int width, int height)
    {
        HRESULT hr = S_OK;

        // --- スワップチェーン記述子の設定 ---
        // バックバッファのフォーマット、リフレッシュレート、出力先ウィンドウを定義
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG) || defined(_DEBUG)
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        // --- デバイスおよびスワップチェーンの同時生成 ---
        // ハードウェアアクセラレーションを利用するデバイスを生成
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            1,
            D3D11_SDK_VERSION,
            &sd,
            &m_pSwapChain,
            &m_pDevice,
            nullptr,
            &m_pContext
        );
        if (FAILED(hr)) return hr;

        // --- レンダーターゲットビュー (RTV) の構築 ---
        // スワップチェーンのバックバッファをパイプラインの描画先としてバインド可能にする
        Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)pBackBuffer.GetAddressOf());
        m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);

        // --- 深度バッファおよび深度ステンシルビュー (DSV) の構築 ---
        // Zバッファアルゴリズムによる隠面消去を実現するための物理リソースを生成
        D3D11_TEXTURE2D_DESC descDepth = {};
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDepth.SampleDesc.Count = 1;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        m_pDevice->CreateTexture2D(&descDepth, nullptr, &m_pDepthStencil);
        m_pDevice->CreateDepthStencilView(m_pDepthStencil.Get(), nullptr, &m_pDSV);

        // --- 出力マージャ (OM) ステージへのバインド ---
        m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), m_pDSV.Get());

        // --- ラスタライザ (RS) ステージのビューポート設定 ---
        D3D11_VIEWPORT vp = { 0, 0, (float)width, (float)height, 0.0f, 1.0f };
        m_pContext->RSSetViewports(1, &vp);

        return S_OK;
    }

    void Graphics::BeginScene(float r, float g, float b, float a)
    {
        // カラーバッファと深度バッファをフレーム開始時に初期化
        float color[4] = { r, g, b, a };
        m_pContext->ClearRenderTargetView(m_pRTV.Get(), color);
        m_pContext->ClearDepthStencilView(m_pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    }

    void Graphics::EndScene()
    {
        // 垂直同期を有効にしてフロントバッファとバックバッファを反転
        m_pSwapChain->Present(1, 0);
    }

    void Graphics::SetRender()
    {
        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->RSSetState(m_pRS.Get());
        m_pContext->OMSetDepthStencilState(m_pDSS.Get(), 1);
	}

    void Graphics::SetTextureAndSampler()
    {
        m_pContext->PSSetShaderResources(0, 1, m_pSRV.GetAddressOf());
        m_pContext->PSSetSamplers(0, 1, m_pSampler.GetAddressOf());
	}

    void Graphics::Update(DirectX::XMMATRIX world, DirectX::XMMATRIX view, DirectX::XMMATRIX proj)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            ConstantBufferData* data = (ConstantBufferData*)mapped.pData;
            data->mWorld = XMMatrixTranspose(world);
            data->mView = XMMatrixTranspose(view);
            data->mProjection = XMMatrixTranspose(proj);
            data->vLightDir = DirectX::XMFLOAT4(-1, -1, 1, 0);
            data->vLightColor = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1);
            data->vAmbient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1);
            
            // --- TODO 1: カメラ位置の転送 ---
            /*
                PBRにおける視線ベクトル(ViewDir)の計算のため、カメラのワールド座標が必要です。
                引数で渡される view 行列の「逆行列」を計算し、その逆行列の第4行（平行移動成分）を
                data->vCameraPos に格納してください。
                ヒント: XMMatrixInverse と invView.r[3]、XMStoreFloat4 を利用します。
            */
            DirectX::XMMATRIX invVew = XMMatrixInverse(nullptr, view);
			DirectX::XMStoreFloat4(&data->vCameraPos, invVew.r[3]);

            m_pContext->Unmap(m_pConstantBuffer.Get(), 0);
        }
    }

    HRESULT Graphics::InitConstantBuffer()
    {
        D3D11_BUFFER_DESC cbd = {};
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.ByteWidth = sizeof(ConstantBufferData);
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        return m_pDevice->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);
    }

    HRESULT Graphics::InitPipelineStates()
    {
        // ラスタライザステート (カリング無効)
        D3D11_RASTERIZER_DESC rd = {};
        rd.CullMode = D3D11_CULL_NONE;
        rd.FillMode = D3D11_FILL_SOLID;
        rd.DepthClipEnable = TRUE;
        m_pDevice->CreateRasterizerState(&rd, &m_pRS);

        // 深度ステンシルステート
        D3D11_DEPTH_STENCIL_DESC dsd = {};
        dsd.DepthEnable = TRUE;
        dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsd.DepthFunc = D3D11_COMPARISON_LESS;
        m_pDevice->CreateDepthStencilState(&dsd, &m_pDSS);

        // サンプラーステート生成
        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        return m_pDevice->CreateSamplerState(&sampDesc, &m_pSampler);
    }
}