#include "Texture.h"
#include <wincodec.h>
#include <vector>

#pragma comment(lib, "Windowscodecs.lib")

namespace Engine
{
    Texture::Texture() {}
    Texture::~Texture() {}

    HRESULT Texture::LoadFromFile(ID3D11Device* pDevice, const std::wstring& filePath)
    {
        HRESULT hr = S_OK;

        // WICコンポーネントの初期化
        Microsoft::WRL::ComPtr<IWICImagingFactory> pFactory;

        // --- TODO 1: WICファクトリの生成 ---
        /* CoCreateInstance を呼び出し、WICイメージングファクトリを生成せよ。
           CLSID: CLSID_WICImagingFactory, IID: IID_PPV_ARGS(&pFactory)
        */
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pFactory)
		);
        if (FAILED(hr)) return hr;

        // デコーダの生成
        Microsoft::WRL::ComPtr<IWICBitmapDecoder> pDecoder;
        hr = pFactory->CreateDecoderFromFilename(
            filePath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
        if (FAILED(hr)) return hr;

        // 最初のフレームを取得
        Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> pFrame;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr)) return hr;

        // フォーマットコンバータの生成 (Direct3D互換の32bpp RGBAへ変換)
        Microsoft::WRL::ComPtr<IWICFormatConverter> pConverter;

        // --- TODO 2: フォーマットコンバータの生成と初期化 ---
        /* 1. pFactory->CreateFormatConverter を呼び出し、pConverter を生成せよ。
           2. pConverter->Initialize を呼び出し、以下のパラメータでフォーマット変換を初期化せよ。
              引数: pFrame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom
        */
        hr = pFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr)) return hr;

        hr = pConverter->Initialize(
            pFrame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
		);
        if (FAILED(hr)) return hr;

        UINT width, height;
        pConverter->GetSize(&width, &height);

        // バイトストリームの計算とバッファアロケーション (RGBA = 4 bytes per pixel)
        UINT bytesPerPixel = 4;
        UINT rowPitch = width * bytesPerPixel;
        UINT imageSize = rowPitch * height;
        std::vector<uint8_t> pixels(imageSize);

        // --- TODO 3: ピクセルデータのメモリコピー ---
        /* pConverter->CopyPixels を呼び出し、デコードされたピクセル配列を pixels バッファへコピーせよ。
           引数: nullptr, rowPitch, imageSize, pixels.data()
        */
        pConverter->CopyPixels(
            nullptr,
            rowPitch,
            imageSize,
            pixels.data()
        );
        if (FAILED(hr)) return hr;

        // --- TODO 4: テクスチャリソースのVRAM転送 ---
        /* D3D11_TEXTURE2D_DESC を構築し、width, height を設定せよ。
           MipLevels=1, ArraySize=1, Format=DXGI_FORMAT_R8G8B8A8_UNORM, SampleDesc.Count=1,
           Usage=D3D11_USAGE_IMMUTABLE, BindFlags=D3D11_BIND_SHADER_RESOURCE とせよ。
           
           D3D11_SUBRESOURCE_DATA initData を構築し、pSysMem=pixels.data(), SysMemPitch=rowPitch とせよ。
           pDevice->CreateTexture2D を呼び出して m_pTexture を生成し、
           pDevice->CreateShaderResourceView を呼び出して m_pSRV を生成せよ。
        */
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width; 
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = pixels.data();
		initData.SysMemPitch = rowPitch;

        hr = pDevice->CreateTexture2D(&desc, &initData, &m_pTexture);
        if (FAILED(hr)) return hr;

		hr = pDevice->CreateShaderResourceView(m_pTexture.Get(), nullptr, &m_pSRV);

        return hr;
    }

    void Texture::Bind(ID3D11DeviceContext* pContext, UINT slot)
    {
        if (!m_pSRV)
        {
            return;
        }
        // 取得した画像リソース（SRV）をパイプラインにセット
        pContext->PSSetShaderResources(slot, 1, m_pSRV.GetAddressOf());
    }

    HRESULT Texture::CreateSolidColor(
        ID3D11Device* pDevice,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a
    )
    {
        if (!pDevice)
        {
            return E_INVALIDARG;
        }

        // 1x1の2Dテクスチャ
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // 4バイトのピクセルデータ (RGBA) を直接メモリ上に用意
        uint32_t pixel = (static_cast<uint32_t>(a) << 24) 
            | (static_cast<uint32_t>(b) << 16)
            | (static_cast<uint32_t>(g) << 8)
            | (static_cast<uint32_t>(r));

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &pixel;
        initData.SysMemPitch = sizeof(uint32_t);

        // GPUバッファとシェーダーリソースビュー (SRV) の生成
        HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pTexture);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = pDevice->CreateShaderResourceView(m_pTexture.Get(), nullptr, &m_pSRV);
        return hr;
    }
}