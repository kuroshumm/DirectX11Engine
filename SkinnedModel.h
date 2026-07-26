#pragma once
#include <wrl/client.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <memory>
#include <string>
#include <map>

#include "SkinnedMesh.h"
#include "IModel.h"
#include "IIKSolver.h"

namespace Engine
{
	class Graphics;
	class Bone;

	class SkinnedModel : public IModel
	{
	public:
		SkinnedModel(Graphics* pGfx);
		~SkinnedModel();

		void SetMaterial(const std::shared_ptr<Material>& pMaterial) override;
		// 指定されたファイルパスからジオメトリデータを抽出し、VRAMへロードする
		HRESULT LoadMesh(const std::string& filePath) override;
		// 保持している全てのメッシュストリームに対するドローコールを発行する
		void Draw() override;
		void Update(float deltaTime) override;
		void UploadPaletteBuffer();
		void ApplyIK(IIKSolver* pSolver, int effectorIdx, const std::vector<int>& chainIndices, const DirectX::XMFLOAT3& targetPos);

		DirectX::BoundingSphere GetLocalBoundingSphere() const override { return m_LocalBoundingSphere; }
		std::vector<Bone>& GetBone() { return m_bones; }
		int GetBoneIndex(const std::string& name) const;

	private:
		Graphics* m_pGfx;

		// ローカル座標系における境界球 (モデル全体を包み込む最小の球体)
		DirectX::BoundingSphere m_LocalBoundingSphere;
		// モデル全体で共有されるマテリアル (シェーダーやテクスチャなどのレンダリングリソース) を保持
		std::shared_ptr<Material> m_pMaterial;

		int m_rootBoneIndex = -1;

		std::vector<std::unique_ptr<SkinnedMesh>> m_skinnedMeshs;
		std::vector<Bone> m_bones;
		std::map <std::string, int> m_boneMapping;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_pPaletteBuffer;
	
	private:
		HRESULT CretePaletteBufferData();
		UINT CalcGlobalBoneIndex(aiBone* pBone);
		DirectX::XMFLOAT4X4 ConvertMatrix(const aiMatrix4x4& aiMat);
		int ProcessNode(aiNode* node, int parentGlobalIndex);
		void UpdateBoneTransform(int boneIndex, DirectX::XMMATRIX parentGlobalTransform);
	};
}