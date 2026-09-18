#pragma once
#include <memory>
#include <wrl/client.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>

#include "Graphics.h"
#include "Components.h"

namespace Engine
{
	class ModelLoader
	{
	public:
		ModelLoader(Graphics* pGfx);

		// FBXファイルからModelAssetを生成（ジオメトリのみ）
		std::shared_ptr<ModelAsset> LoadModelAsset(const std::string& filePath);
		
		// 一括ロード：ModelAssetとSkeltonAssetを同時に生成
		struct LoadResult
		{
			bool isSuccess = false;
			std::shared_ptr<ModelAsset> modelAsset;
			std::shared_ptr<SkeltonAsset> skeltonAsset;
		};
		LoadResult Load(const std::string& filePath);

		// GPU定数バッファ（PaletteBuffer）の生成ヘルパー
		static HRESULT CreatePaletteBuffer(
			ID3D11Device* pDevice,
			Microsoft::WRL::ComPtr<ID3D11Buffer>& outBuffer
		)
		{
			D3D11_BUFFER_DESC vbd = {};
			vbd.ByteWidth = sizeof(PaletteBufferData);
			vbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			vbd.Usage = D3D11_USAGE_DYNAMIC;
			vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			HRESULT hr = pDevice->CreateBuffer(&vbd, nullptr, &outBuffer);
			return hr;
		}

	private:
		Graphics* m_pGfx;

		template<typename VertexType>
		SubMesh CreateSubMesh(const std::vector<VertexType>& vertices, const std::vector<WORD>& indices)
		{
			SubMesh subMesh;

			// --- 頂点バッファ (VertexBuffer) の生成 ---
			// 生成後は変更されないため D3D11_USAGE_IMMUTABLE を使用し、PCIe経由の転送を最小化
			D3D11_BUFFER_DESC vbd = {};
			vbd.ByteWidth = sizeof(VertexType) * (UINT)vertices.size();
			vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vbd.Usage = D3D11_USAGE_IMMUTABLE;
			D3D11_SUBRESOURCE_DATA vinit = { vertices.data() };
			HRESULT hr = m_pGfx->GetDevice()->CreateBuffer(&vbd, &vinit, &subMesh.vertexBuffer);

			// --- インデックスバッファ (IndexBuffer) の生成 ---
			D3D11_BUFFER_DESC ibd = {};
			ibd.ByteWidth = sizeof(WORD) * (UINT)indices.size();
			ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			ibd.Usage = D3D11_USAGE_IMMUTABLE;
			D3D11_SUBRESOURCE_DATA iinit = { indices.data() };
			hr = m_pGfx->GetDevice()->CreateBuffer(&ibd, &iinit, &subMesh.indexBuffer);

			subMesh.indexCount = (UINT)indices.size();
			subMesh.vertexStride = sizeof(VertexType);

			return subMesh;
		}
	};
}