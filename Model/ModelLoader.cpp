#include "ModelLoader.h"
#include <fstream>

namespace
{
	using namespace Engine;

	DirectX::XMFLOAT4X4 ConvertMatrix(const aiMatrix4x4& aiMat)
	{
		DirectX::XMFLOAT4X4 dxMat;
		// 転置せず、Assimpのメモリ配列の通りに素直に完全コピーします
		dxMat._11 = aiMat.a1; dxMat._12 = aiMat.b1; dxMat._13 = aiMat.c1; dxMat._14 = aiMat.d1;
		dxMat._21 = aiMat.a2; dxMat._22 = aiMat.b2; dxMat._23 = aiMat.c2; dxMat._24 = aiMat.d2;
		dxMat._31 = aiMat.a3; dxMat._32 = aiMat.b3; dxMat._33 = aiMat.c3; dxMat._34 = aiMat.d3;
		dxMat._41 = aiMat.a4; dxMat._42 = aiMat.b4; dxMat._43 = aiMat.c4; dxMat._44 = aiMat.d4;
		return dxMat;
	}

	UINT CalcGlobalBoneIndex(aiBone* pBone, SkeltonAsset& skeltonAsset)
	{
		std::string boneName = pBone->mName.C_Str();
		int globalBoneIndex = 0;

		// 辞書にこの名前が登録されているかチェック
		if (skeltonAsset.boneMapping.find(boneName) == skeltonAsset.boneMapping.end())
		{
			// 未登録（初めて見るボーン）の場合
			Bone newBone = {};
			newBone.name = boneName;

			// (ここで inverseBindMatrix 等もAssimpの bone->mOffsetMatrix から取得して変換・代入します)
			newBone.InverseBindMatrix = ConvertMatrix(pBone->mOffsetMatrix);
			DirectX::XMStoreFloat4x4(&newBone.GlobalTransformMatrix, DirectX::XMMatrixIdentity());

			// 配列の末尾に追加
			skeltonAsset.initialBones.push_back(newBone);

			// 今追加した場所（配列のサイズ - 1）が、このボーンのグローバルインデックスとなる
			globalBoneIndex = static_cast<int>(skeltonAsset.initialBones.size() - 1);

			// 辞書に「この名前のボーンは x 番目にある」と記憶させる
			skeltonAsset.boneMapping[boneName] = globalBoneIndex;
		}
		else
		{
			// 登録済み（別のメッシュですでに登録されたボーン）の場合
			// 辞書からグローバルインデックスを引っ張り出してくるだけ
			globalBoneIndex = skeltonAsset.boneMapping[boneName];
		}

		return globalBoneIndex;
	}



	int ProcessNode(
		aiNode* node,
		int parentIndex,
		std::vector<Bone>& bones,
		std::unordered_map<std::string, int>& mapping
	)
	{
		std::string nodeName = node->mName.C_Str();
		int currentIndex = -1;

		// 既に登録されてるか
		if (mapping.find(nodeName) != mapping.end())
		{
			currentIndex = mapping[nodeName];
		}
		else
		{
			// ウェイトを持たない中間ノードも関節として追加する
			Bone newBone = {};
			newBone.name = nodeName;
			DirectX::XMStoreFloat4x4(&newBone.InverseBindMatrix, DirectX::XMMatrixIdentity());
			bones.push_back(newBone);
			currentIndex = bones.size() - 1;
			mapping[nodeName] = currentIndex;
		}

		bones[currentIndex].parentIndex = parentIndex;
		bones[currentIndex].LocalTransformMatrix = ConvertMatrix(node->mTransformation);
		// LocalTransformMatrix を分解してキャッシュに保存（ロード時に1回だけ）
		DirectX::XMVECTOR s, r, t;
		DirectX::XMMatrixDecompose(&s, &r, &t,
			DirectX::XMLoadFloat4x4(&bones[currentIndex].LocalTransformMatrix));
		DirectX::XMStoreFloat3(&bones[currentIndex].defaultPos, t);
		DirectX::XMStoreFloat4(&bones[currentIndex].defaultRot, r);
		DirectX::XMStoreFloat3(&bones[currentIndex].defaultScale, s);

		if (parentIndex != -1)
		{
			bones[parentIndex].childIndices.push_back(currentIndex);
		}

		for (UINT nodeIndex = 0, max = node->mNumChildren; nodeIndex < max; ++nodeIndex)
		{
			ProcessNode(node->mChildren[nodeIndex], currentIndex, bones, mapping);
		}

		return currentIndex;
	}

	std::vector<Vertex> CreateMeshVertices(const aiMesh& mesh)
	{
		std::vector<Vertex> vertices;

		for (UINT v = 0, max = mesh.mNumVertices; v < max; ++v)
		{
			Vertex vertex;
			vertex.Pos = { mesh.mVertices[v].x, mesh.mVertices[v].y, mesh.mVertices[v].z };
			vertex.Normal = mesh.HasNormals()
				? DirectX::XMFLOAT3(mesh.mNormals[v].x, mesh.mNormals[v].y, mesh.mNormals[v].z)
				: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.Tangent = mesh.HasTangentsAndBitangents()
				? DirectX::XMFLOAT3(mesh.mTangents[v].x, mesh.mTangents[v].y, mesh.mTangents[v].z)
				: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = mesh.HasTextureCoords(0)
				? DirectX::XMFLOAT2(mesh.mTextureCoords[0][v].x, mesh.mTextureCoords[0][v].y)
				: DirectX::XMFLOAT2(0.0f, 0.0f);

			// Colorは現状白固定とする
			vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			vertices.push_back(vertex);
		}

		return vertices;
	}

	std::vector<SkinnedVertex> CreateSkinnedMeshVertices(const aiMesh& mesh)
	{
		std::vector<SkinnedVertex> vertices;

		for (UINT v = 0, max = mesh.mNumVertices; v < max; ++v)
		{
			SkinnedVertex vertex;
			vertex.Pos = { mesh.mVertices[v].x, mesh.mVertices[v].y, mesh.mVertices[v].z };
			vertex.Normal = mesh.HasNormals()
				? DirectX::XMFLOAT3(mesh.mNormals[v].x, mesh.mNormals[v].y, mesh.mNormals[v].z)
				: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.Tangent = mesh.HasTangentsAndBitangents()
				? DirectX::XMFLOAT3(mesh.mTangents[v].x, mesh.mTangents[v].y, mesh.mTangents[v].z)
				: DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
			vertex.TexCoord = mesh.HasTextureCoords(0)
				? DirectX::XMFLOAT2(mesh.mTextureCoords[0][v].x, mesh.mTextureCoords[0][v].y)
				: DirectX::XMFLOAT2(0.0f, 0.0f);

			// Colorは現状白固定とする
			vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
			vertex.BlendIndices = { 0, 0, 0, 0 };
			vertex.BlendWeight = { 0.0f, 0.0f, 0.0f, 0.0f };

			vertices.push_back(vertex);
		}

		return vertices;
	}

	void LoadSkinnedMeshVerticesBlendWeight(
		const aiMesh& mesh,
		std::vector<SkinnedVertex>& vertices,
		SkeltonAsset& skeltonAsset)
	{
		std::vector<int> weightCount(mesh.mNumVertices, 0);
		for (UINT boneIndex = 0, max = mesh.mNumBones; boneIndex < max; ++boneIndex)
		{
			// 全てのボーンに対して影響度を計算する
			aiBone* bone = mesh.mBones[boneIndex];
			for (UINT weightIndex = 0, weightNum = bone->mNumWeights; weightIndex < weightNum; ++weightIndex)
			{
				UINT vertexId = bone->mWeights[weightIndex].mVertexId;
				float weight = bone->mWeights[weightIndex].mWeight;

				// 計算する影響するボーンは４つに制限（GPU側のXMUINT4, XMFLOAT4が4要素のため）
				int slot = weightCount[vertexId];
				if (slot >= 4)
				{
					continue;
				}

				// 影響するボーンのインデックス
				reinterpret_cast<UINT*>(&vertices[vertexId].BlendIndices)[slot] = CalcGlobalBoneIndex(bone, skeltonAsset);
				// 影響するボーンの影響度
				reinterpret_cast<float*>(&vertices[vertexId].BlendWeight)[slot] = weight;

				weightCount[vertexId]++;
			}
		}
	}

	std::vector<WORD> CreateMeshIndex(const aiMesh& mesh)
	{
		std::vector<WORD> indices;

		for (UINT f = 0; f < mesh.mNumFaces; ++f)
		{
			const aiFace& face = mesh.mFaces[f];
			for (UINT idx = 0; idx < face.mNumIndices; ++idx)
			{
				indices.push_back(static_cast<WORD>(face.mIndices[idx]));
			}
		}

		return indices;
	}

	

	void DumpNodeHierarchy(aiNode* node, int depth, std::ofstream& ofs)
	{
		if (!node) return;
		std::string indent(depth * 2, ' ');
		ofs << indent << "- Node: '" << node->mName.C_Str() << "', Meshes: " << node->mNumMeshes << "\n";
		aiVector3D scaling, position;
		aiQuaternion rotation;
		node->mTransformation.Decompose(scaling, rotation, position);
		ofs << indent << "  Pos: (" << position.x << ", " << position.y << ", " << position.z << ")"
			<< " Rot: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ", " << rotation.w << ")"
			<< " Scale: (" << scaling.x << ", " << scaling.y << ", " << scaling.z << ")\n";

		if (node->mNumMeshes > 0)
		{
			ofs << indent << "  MeshIndices: ";
			for (unsigned int i = 0; i < node->mNumMeshes; ++i)
			{
				ofs << node->mMeshes[i] << " ";
			}
			ofs << "\n";
		}

		for (unsigned int i = 0; i < node->mNumChildren; ++i)
		{
			DumpNodeHierarchy(node->mChildren[i], depth + 1, ofs);
		}
	}
}

namespace Engine
{
	ModelLoader::ModelLoader(Graphics* pGfx) 
		: m_pGfx(pGfx)
	{
	}

	std::shared_ptr<ModelAsset> ModelLoader::LoadModelAsset(const std::string& filePath)
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
			return nullptr;
		}

		std::shared_ptr<ModelAsset> pModelAsset = std::make_shared<ModelAsset>();

		// シーン内の全メッシュを反復処理しSubMeshへコンバート
		for (UINT meshIndex = 0, max = scene->mNumMeshes; meshIndex < max; ++meshIndex)
		{
			aiMesh* mesh = scene->mMeshes[meshIndex];

			// 頂点データの抽出
			std::vector<Vertex> vertices = CreateMeshVertices(*mesh);
			// インデックスデータの抽出
			std::vector<WORD> indices = CreateMeshIndex(*mesh);
			// SubMeshの作成
			SubMesh subMesh = CreateSubMesh<Vertex>(vertices, indices);

			// DirectXMathの機能を使って、全頂点を包み込む初期スフィアを自動計算
            // (頂点のメモリ先頭ポインタ、頂点数、1頂点あたりのバイトサイズを渡す)
			DirectX::BoundingSphere::CreateFromPoints(
				pModelAsset->totalLocalBounds,
				vertices.size(),
				&vertices[0].Pos,
				sizeof(Vertex)
			);

			pModelAsset->subMeshs.push_back(std::move(subMesh));
		}

		return pModelAsset;
	}
	
	ModelLoader::LoadResult ModelLoader::Load(const std::string& filePath)
	{
		Assimp::Importer importer;
		ModelLoader::LoadResult loadResult;
		loadResult.modelAsset = std::make_shared<ModelAsset>();
		loadResult.skeltonAsset = std::make_shared<SkeltonAsset>();

		unsigned int flags = aiProcess_Triangulate
			| aiProcess_ConvertToLeftHanded
			| aiProcess_LimitBoneWeights
			| aiProcess_CalcTangentSpace
			| aiProcess_GenSmoothNormals;
		const aiScene* scene = importer.ReadFile(filePath, flags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			OutputDebugStringA(importer.GetErrorString());
			return loadResult;
		}

		{
			std::ofstream ofs("C:/Users/kuros/.gemini/antigravity-ide/brain/e39f8484-d67b-4eae-83a6-5d586606eb46/scratch/node_dump.txt");
			ofs << "=== FBX Node Hierarchy: " << filePath << " ===\n";
			DumpNodeHierarchy(scene->mRootNode, 0, ofs);
		}

		for (UINT meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
		{
			aiMesh* mesh = scene->mMeshes[meshIndex];

			// 頂点データの抽出
			std::vector<SkinnedVertex> vertices = CreateSkinnedMeshVertices(*mesh);
			// ボーンの影響度を計算
			LoadSkinnedMeshVerticesBlendWeight(*mesh, vertices, *loadResult.skeltonAsset.get());
			// インデックスデータの抽出
			std::vector<WORD> indices = CreateMeshIndex(*mesh);

			// SubMeshの作成
			SubMesh subMesh = CreateSubMesh<SkinnedVertex>(vertices, indices);

			// DirectXMathの機能を使って、全頂点を包み込む初期スフィアを自動計算
			// (頂点のメモリ先頭ポインタ、頂点数、1頂点あたりのバイトサイズを渡す)
			DirectX::BoundingSphere::CreateFromPoints(
				loadResult.modelAsset->totalLocalBounds,
				vertices.size(),
				&vertices[0].Pos,
				sizeof(SkinnedVertex)
			);

			loadResult.modelAsset->subMeshs.push_back(std::move(subMesh));
		}

		if (scene->mRootNode)
		{
			loadResult.skeltonAsset->rootBoneIndex = ProcessNode(
				scene->mRootNode,
				-1,
				loadResult.skeltonAsset->initialBones,
				loadResult.skeltonAsset->boneMapping
			);
		}

		loadResult.isSuccess = true;
		return loadResult;
	}

}

