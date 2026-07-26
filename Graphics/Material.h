#pragma once
#include <memory>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <d3d11.h>
#include "Graphics.h"
#include "Shader.h"
#include "Texture.h"
#include "Common.h"

namespace Engine
{
	class Material
	{
	public:
		Material();
		~Material();

		void SetShader(std::weak_ptr<Shader> pShader);
		void SetTexture(const std::shared_ptr<Texture>& pTexture);

		// PBRパラメータ設定用
		void SetBaseColor(float r, float g, float b, float a);
		void SetMetallic(float metallic);
		void SetRoughness(float roughness);

		// バッファ生成
		HRESULT CreateBuffer(ID3D11Device* device);

		void Bind(Graphics* pGfx);

	private:
		std::weak_ptr<Shader> m_pShader;
		std::shared_ptr<Texture> m_pTexture;

		// --- 第12回: PBRマテリアル定数バッファ ---
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pMaterialCB;
		DirectX::XMFLOAT4 m_BaseColor;
		float m_Metallic;
		float m_Roughness;
	};
}
