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

		// 指定されたファイルパスからテクスチャをロードし、VRAMへ転送する
		HRESULT LoadFromFile(ID3D11Device* pDevice, const std::wstring& filePath);

		ID3D11ShaderResourceView* GetSRV() { return m_pSRV.Get(); }

	private:
		Microsoft::WRL::ComPtr<ID3D11Texture2D>            m_pTexture;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>   m_pSRV;
	};
}