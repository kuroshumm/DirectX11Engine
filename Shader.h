#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace Engine
{
	class Shader
	{
	public:
		Shader();
		~Shader();

		HRESULT Initialize(ID3D11Device* pDeveice);

		void Render(ID3D11DeviceContext* pContext);

		HRESULT CompilePixelShader(const wchar_t* filePath, const char* entryPoint);
		HRESULT CompileVertexShader(const wchar_t* filePath, const char* entryPoint);
		HRESULT CreateInputLayout();

	private:
		Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pPS;
		Microsoft::WRL::ComPtr<ID3DBlob>               m_psBlob;
		Microsoft::WRL::ComPtr<ID3DBlob>               m_vsBlob;

		Microsoft::WRL::ComPtr<ID3D11InputLayout>      m_pLayout;
	};
}