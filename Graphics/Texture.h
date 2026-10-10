#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>

namespace Engine
{
	class Texture
	{
	public:
		Texture();
		~Texture();

		ID3D11ShaderResourceView* GetSRV() { return m_pSRV.Get(); }

		// 指定されたファイルパスからテクスチャをロードし、VRAMへ転送する
		HRESULT LoadFromFile(ID3D11Device* pDevice, const std::wstring& filePath);
		// ロードしたテクスチャをパイプラインにセット
		void Bind(ID3D11DeviceContext* pContext, UINT slot);

		HRESULT CreateSolidColor(
			ID3D11Device* pDevice,
			uint8_t r = 255,
			uint8_t g = 255,
			uint8_t b = 255,
			uint8_t a = 255
		);

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D>            m_pTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   m_pSRV;
	};
}