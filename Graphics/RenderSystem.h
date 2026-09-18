#pragma once

#include <entt/entt.hpp>

#include "Graphics.h"
#include "Camera.h"
#include "Components.h"

namespace Engine
{
	class RenderSystem
	{
	public:
		// 初期化時に定数バッファを1回だけ生成
		HRESULT Initialize(ID3D11Device* pDevice);
		// 
		void Render(entt::registry& registry, Graphics& pGfx);
	private:
		void UpdateMaterial(MaterialComponent& material, ID3D11DeviceContext& context);

	private:
		// マテリアルパラメータ転送用の共有定数バッファ
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pMaterialCB;
	};
}