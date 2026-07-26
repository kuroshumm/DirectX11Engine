#include <algorithm>
#include <stdexcept>
#include <fstream>
#include "SkinnedModel.h"
#include "Common.h"
#include "Material.h"

namespace Engine
{
	void DumpNodeHierarchy(aiNode* node, int depth) {
		if(!node) return;
		std::string str;
		for(int i=0; i<depth; ++i) str += "  ";
		str += "- ";
		str += node->mName.C_Str();
		str += "\n";
		OutputDebugStringA(str.c_str());
		for(unsigned int i=0; i<node->mNumChildren; ++i) {
			DumpNodeHierarchy(node->mChildren[i], depth+1);
		}
	}

	SkinnedModel::SkinnedModel(Graphics* pGfx) : m_pGfx(pGfx)
	{
		// 行列パレットの定数バッファ生成
		CretePaletteBufferData();
	}
	SkinnedModel::~SkinnedModel() {}

	void SkinnedModel::SetMaterial(const std::shared_ptr<Material>& pMaterial)
	{
		m_pMaterial = pMaterial;
	}

	HRESULT SkinnedModel::LoadMesh(const std::string& filePath)
	{
		Assimp::Importer importer;

		unsigned int flags = aiProcess_Triangulate
			| aiProcess_ConvertToLeftHanded
			| aiProcess_CalcTangentSpace
			| aiProcess_GenSmoothNormals;
		const aiScene* scene = importer.ReadFile(filePath, flags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			OutputDebugStringA(importer.GetErrorString());
			return E_FAIL;
		}

		{
			OutputDebugStringA("\n=== FBX Node Hierarchy ===\n");
			DumpNodeHierarchy(scene->mRootNode, 0);
			OutputDebugStringA("==========================\n\n");
		}

		m_skinnedMeshs.clear();
		m_bones.clear();

		// 
		for (UINT meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
		{
			aiMesh* mesh = scene->mMeshes[meshIndex];
			std::vector<SkinnedVertex> vertices;
			std::vector<WORD> indices;

			// 頂点データを取得
			for (UINT v = 0; v < mesh->mNumVertices; ++v)
			{
				SkinnedVertex vertex;
				vertex.Pos = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
				vertex.Normal = mesh->HasNormals()
					? DirectX::XMFLOAT3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z)
					: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
				vertex.Tangent = mesh->HasTangentsAndBitangents()
					? DirectX::XMFLOAT3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z)
					: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
				vertex.TexCoord = mesh->HasTextureCoords(0)
					? DirectX::XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y)
					: DirectX::XMFLOAT2(0.0f, 0.0f);
				vertex.BlendIndices = { 0, 0, 0, 0 };
				vertex.BlendWeight = { 0.0f, 0.0f, 0.0f, 0.0f };

				// Colorは現状白固定とする
				vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
				vertices.push_back(vertex);
			}

			// 頂点データのボーンに関する情報を取得
			std::vector<int> weightCount(vertices.size(),0);
			for (UINT boneIndex = 0, boneCount = mesh->mNumBones; boneIndex < boneCount; ++boneIndex)
			{
				aiBone* bone = mesh->mBones[boneIndex];
				for (UINT weightIndex = 0, weightNum = bone->mNumWeights; weightIndex < weightNum; ++weightIndex)
				{
					UINT vertexId = bone->mWeights[weightIndex].mVertexId;
					float weight = bone->mWeights[weightIndex].mWeight;

					int slot = weightCount[vertexId];

					if (slot < 4)
					{
						reinterpret_cast<UINT*>(&vertices[vertexId].BlendIndices)[slot] = CalcGlobalBoneIndex(bone);
						reinterpret_cast<float*>(&vertices[vertexId].BlendWeight)[slot] = weight;

						weightCount[vertexId]++;
					}
				}
			}

			for (UINT faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
			{
				aiFace face = mesh->mFaces[faceIndex];
				for (UINT idx = 0; idx < face.mNumIndices; ++idx)
				{
					indices.push_back(static_cast<WORD>(face.mIndices[idx]));
				}
			}

			std::unique_ptr<SkinnedMesh> pSkinnedMesh = std::make_unique<SkinnedMesh>(m_pGfx);
			if (SUCCEEDED(pSkinnedMesh->Create(vertices, indices)))
			{
				m_skinnedMeshs.push_back(std::move(pSkinnedMesh));
			}

			// DirectXMathの機能を使って、全頂点を包み込む初期スフィアを自動計算
			// (頂点のメモリ先頭ポインタ、頂点数、1頂点あたりのバイトサイズを渡す)
			DirectX::BoundingSphere::CreateFromPoints(
				m_LocalBoundingSphere,
				vertices.size(),
				&vertices[0].Pos,
				sizeof(Vertex)
			);
		}

		if (scene->mRootNode)
		{
			m_rootBoneIndex = ProcessNode(scene->mRootNode, -1);
		}

		return S_OK;
	}

	void SkinnedModel::Draw()
	{
		if (!m_pMaterial)
		{
			return;
		}
		m_pMaterial->Bind(m_pGfx);

		if (!m_pPaletteBuffer)
		{
			return;
		}
		ID3D11DeviceContext* context = m_pGfx->GetContext();
		context->VSSetConstantBuffers(1, 1, m_pPaletteBuffer.GetAddressOf());

		for (auto& mesh : m_skinnedMeshs)
		{
			mesh->Draw();
		}
	}

	void SkinnedModel::Update(float deltaTime)
	{
		if (m_rootBoneIndex == -1) return;

		// ルートノードから再帰的にすべてのボーンへFKを計算し伝搬させる
		UpdateBoneTransform(m_rootBoneIndex, DirectX::XMMatrixIdentity());
	}

	void SkinnedModel::UploadPaletteBuffer()
	{
		if (m_bones.empty()) return;
		if (!m_pPaletteBuffer) return;

		// 全ての計算が終わった行列を使って最終スキニング行列を作りパレットに書き込む
		// 計算したスキニング行列をGPUに渡す
		ID3D11DeviceContext* pContext = m_pGfx->GetContext();
		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(pContext->Map(m_pPaletteBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			PaletteBufferData* data = (PaletteBufferData*)mapped.pData;

			int maxBoneNum = std::min(static_cast<int>(m_bones.size()), MAX_BONES);
			for (int boneIndex = 0; boneIndex < maxBoneNum; ++boneIndex)
			{
				Bone& bone = m_bones[boneIndex];

				// 3. 【グローバル行列の算出（FK）】
				// 自身のローカルアニメーション × 親の絶対座標 ＝ 自身の絶対座標（足元基準の ModelTransformMatrix）
				DirectX::XMMATRIX globalBoneTransform = DirectX::XMLoadFloat4x4(&bone.GlobalTransformMatrix);

				// 4. 【最終スキニング行列の算出】
				// 頂点をボーンのローカル空間（関節の付け根）に引き戻すための初期逆行列
				DirectX::XMMATRIX inverseBindMat = DirectX::XMLoadFloat4x4(&bone.InverseBindMatrix);
				// 引き戻し（InverseBind）× 新しい絶対座標（globalBoneTransform）
				// これにより「頂点を関節へ引き戻す -> 回転させる -> モデル空間へ戻す」という一連の変形行列が完成する
				DirectX::XMMATRIX skinningMat = DirectX::XMMatrixMultiply(inverseBindMat, globalBoneTransform);

				// 5. 【パレットへの格納】
				// HLSL（頂点シェーダー）側は列優先（Column-Major）を期待するため、転置（Transpose）して定数バッファ用配列へ格納する
				data->Bones[boneIndex] = DirectX::XMMatrixTranspose(skinningMat);
			}

			pContext->Unmap(m_pPaletteBuffer.Get(), 0);
		}
	}

	int SkinnedModel::GetBoneIndex(const std::string& name) const
	{
		// マップからボーン名を検索する
		auto it = m_boneMapping.find(name);

		// 発見できた場合は、対応するインデックス（int）を返す
		if (it != m_boneMapping.end())
		{
			return it->second;
		}

		// 見つからなかった場合は -1 を返す
		return -1;
	}

	HRESULT SkinnedModel::CretePaletteBufferData()
	{
		D3D11_BUFFER_DESC vbd = {};
		vbd.ByteWidth = sizeof(PaletteBufferData);
		vbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		HRESULT hr = m_pGfx->GetDevice()->CreateBuffer(&vbd, nullptr, &m_pPaletteBuffer);
		return hr;
	}

	UINT SkinnedModel::CalcGlobalBoneIndex(aiBone* pBone)
	{
		std::string boneName = pBone->mName.C_Str();
		int globalBoneIndex = 0;

		// 辞書にこの名前が登録されているかチェック
		if (m_boneMapping.find(boneName) == m_boneMapping.end())
		{
			// 未登録（初めて見るボーン）の場合
			Bone newBone = {};
			newBone.name = boneName;
			
			// (ここで inverseBindMatrix 等もAssimpの bone->mOffsetMatrix から取得して変換・代入します)
			newBone.InverseBindMatrix = ConvertMatrix(pBone->mOffsetMatrix);
			DirectX::XMStoreFloat4x4(&newBone.GlobalTransformMatrix, DirectX::XMMatrixIdentity());

			// 配列の末尾に追加
			m_bones.push_back(newBone);

			// 今追加した場所（配列のサイズ - 1）が、このボーンのグローバルインデックスとなる
			globalBoneIndex = m_bones.size() - 1;

			// 辞書に「この名前のボーンは x 番目にある」と記憶させる
			m_boneMapping[boneName] = globalBoneIndex;
		}
		else
		{
			// 登録済み（別のメッシュですでに登録されたボーン）の場合
			// 辞書からグローバルインデックスを引っ張り出してくるだけ
			globalBoneIndex = m_boneMapping[boneName];
		}

		return globalBoneIndex;
	}

	DirectX::XMFLOAT4X4 SkinnedModel::ConvertMatrix(const aiMatrix4x4& aiMat)
	{
		DirectX::XMFLOAT4X4 dxMat;
		// 転置せず、Assimpのメモリ配列の通りに素直に完全コピーします
		dxMat._11 = aiMat.a1; dxMat._12 = aiMat.b1; dxMat._13 = aiMat.c1; dxMat._14 = aiMat.d1;
		dxMat._21 = aiMat.a2; dxMat._22 = aiMat.b2; dxMat._23 = aiMat.c2; dxMat._24 = aiMat.d2;
		dxMat._31 = aiMat.a3; dxMat._32 = aiMat.b3; dxMat._33 = aiMat.c3; dxMat._34 = aiMat.d3;
		dxMat._41 = aiMat.a4; dxMat._42 = aiMat.b4; dxMat._43 = aiMat.c4; dxMat._44 = aiMat.d4;
		return dxMat;
	}

	int SkinnedModel::ProcessNode(aiNode* node, int parentGlobalIndex)
	{
		std::string nodeName = node->mName.C_Str();
		int currentIndex = -1;

		// 既に登録されてるか
		if (m_boneMapping.find(nodeName) != m_boneMapping.end())
		{
			currentIndex = m_boneMapping[nodeName];
		}
		else
		{
			// ウェイトを持たない中間ノードも関節として追加する
			Bone newBone = {};
			newBone.name = nodeName;
			DirectX::XMStoreFloat4x4(&newBone.InverseBindMatrix, DirectX::XMMatrixIdentity());
			m_bones.push_back(newBone);
			currentIndex = m_bones.size() - 1;
			m_boneMapping[nodeName] = currentIndex;
		}

		m_bones[currentIndex].parentIndex = parentGlobalIndex;
		m_bones[currentIndex].LocalTransformMatrix = ConvertMatrix(node->mTransformation);
		m_bones[currentIndex].DefaultLocalMatrix = m_bones[currentIndex].LocalTransformMatrix;

		if (parentGlobalIndex != -1)
		{
			m_bones[parentGlobalIndex].childIndices.push_back(currentIndex);
		}

		for (UINT nodeIndex = 0, max = node->mNumChildren; nodeIndex < max; ++nodeIndex)
		{
			ProcessNode(node->mChildren[nodeIndex], currentIndex);
		}

		return currentIndex;
	}

	void SkinnedModel::UpdateBoneTransform(int boneIndex, DirectX::XMMATRIX parentGlocbalTransform)
	{
		Bone& bone = m_bones[boneIndex];

		DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&bone.LocalTransformMatrix);
		DirectX::XMMATRIX global = DirectX::XMMatrixMultiply(local, parentGlocbalTransform);
		DirectX::XMStoreFloat4x4(&bone.GlobalTransformMatrix, global);

		// 子供たちへ再帰的にFKを伝搬させる
		for (int childIndex : bone.childIndices)
		{
			UpdateBoneTransform(childIndex, global);
		}
	}
}

