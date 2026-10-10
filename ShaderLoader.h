#pragma once

#include <memory>
#include <d3d11.h>

#include "Shader.h"

namespace Engine
{
	class ShaderLoader
	{
	public:
		static std::shared_ptr<Shader> LoadSkinned(
			ID3D11Device* pDevice,
			const wchar_t* vsPath,
			const wchar_t* psPath
		);

		static std::shared_ptr<Shader> LoadStatic(
			ID3D11Device* pDevice,
			const wchar_t* vsPath,
			const wchar_t* psPath
		);
	};
}

