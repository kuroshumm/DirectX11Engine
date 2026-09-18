#pragma once
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Engine
{
    class TextRenderer
    {
    public:
        TextRenderer() = default;
        ~TextRenderer() = default;

        HRESULT Initialize(IDXGISwapChain* pSwapChain)
        {
            HRESULT hr = S_OK;

            // Direct2D ファクトリの作成
            hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_pD2DFactory.GetAddressOf());
            if (FAILED(hr)) return hr;

            // DirectWrite ファクトリの作成
            hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(m_pDWriteFactory.GetAddressOf()));
            if (FAILED(hr)) return hr;

            // テキストフォーマット（フォント・サイズ・右揃え）の作成
            hr = m_pDWriteFactory->CreateTextFormat(
                L"Consolas",
                nullptr,
                DWRITE_FONT_WEIGHT_BOLD,
                DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                20.0f, // フォントサイズ
                L"ja-jp",
                m_pTextFormat.GetAddressOf()
            );
            if (FAILED(hr)) return hr;

            // 右揃えに設定
            m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

            // スワップチェーンのバックバッファから Direct2D レンダーターゲットを作成
            Microsoft::WRL::ComPtr<IDXGISurface> pBackBuffer;
            hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
            if (FAILED(hr)) return hr;

            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE)
            );

            hr = m_pD2DFactory->CreateDxgiSurfaceRenderTarget(
                pBackBuffer.Get(),
                &props,
                m_pRenderTarget.GetAddressOf()
            );
            if (FAILED(hr)) return hr;

            // 白文字ブラシの作成
            hr = m_pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                m_pBrush.GetAddressOf()
            );

            return hr;
        }

        void RenderTextRightTop(const std::wstring& text)
        {
            if (!m_pRenderTarget) return;
            m_pRenderTarget->BeginDraw();
            // ★ レンダーターゲットから実際の画面サイズ (幅と高さ) を自動取得！
            D2D1_SIZE_F size = m_pRenderTarget->GetSize();
            // 画面右上にぴったり収まるように座標を計算 (右端から 260px, 上端から 10px)
            D2D1_RECT_F rect = D2D1::RectF(
                size.width - 260.0f, // 左端 (例: 800 - 260 = 540px)
                10.0f,               // 上端
                size.width - 10.0f,  // 右端 (例: 800 - 10 = 790px)
                150.0f               // 下端
            );
            m_pRenderTarget->DrawTextW(
                text.c_str(),
                static_cast<UINT32>(text.length()),
                m_pTextFormat.Get(),
                &rect,
                m_pBrush.Get()
            );
            m_pRenderTarget->EndDraw();
        }

    private:
        Microsoft::WRL::ComPtr<ID2D1Factory>          m_pD2DFactory;
        Microsoft::WRL::ComPtr<IDWriteFactory>        m_pDWriteFactory;
        Microsoft::WRL::ComPtr<IDWriteTextFormat>     m_pTextFormat;
        Microsoft::WRL::ComPtr<ID2D1RenderTarget>     m_pRenderTarget;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>  m_pBrush;
    };
}