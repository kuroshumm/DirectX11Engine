#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>

namespace Engine
{
	Model::Model(Graphics* pGfx) : m_pGfx(pGfx)
    {
    }
    Model::~Model() {}

    HRESULT Model::LoadMesh(const std::string& filePath)
    {
        Assimp::Importer importer;

        // --- TODO 1: Assimpによるファイルのパース ---
        /* importer.ReadFile を呼び出し、外部ファイルをロードせよ。
           フラグ: aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals
           戻り値の const aiScene* を scene 変数に格納せよ。
        */
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

        // シーン内の全メッシュを反復処理し、エンジン固有のMeshオブジェクトへコンバート
        for (UINT i = 0; i < scene->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[i];
            std::vector<Vertex> vertices;
            std::vector<WORD> indices;

            // --- TODO 2: 頂点データの抽出 ---
            /* mesh->mNumVertices の回数だけループを回し、
               mVertices (位置), mNormals (法線), mTangents (接線), mTextureCoords[0] (UV)
               の各成分を抽出して Vertex 構造体を構築し、vertices ベクタへ push_back せよ。
               ※ポインタのNULLチェック(HasNormals, HasTangentsAndBitangents, HasTextureCoords等)を推奨する。
            */
            for (UINT v = 0; v < mesh->mNumVertices; ++v)
            {
                Vertex vertex;
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

                // Colorは現状白固定とする
                vertex.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
                vertices.push_back(vertex);
            }

            // --- TODO 3: インデックスデータの抽出 ---
            /* mesh->mNumFaces の回数だけループを回し、
               各 aiFace オブジェクト内の mIndices 配列の要素を WORD 型にキャストして
               indices ベクタへ push_back せよ。
            */
            for (UINT f = 0; f < mesh->mNumFaces; ++f)
            {
                aiFace face = mesh->mFaces[f];
                for (UINT idx = 0; idx < face.mNumIndices; ++idx)
                {
                    indices.push_back(static_cast<WORD>(face.mIndices[idx]));
				}
            }

            // 抽出したデータから D3D11Buffer を生成し、リストへ追加
            auto newMesh = std::make_unique<Mesh>(m_pGfx);
            if (SUCCEEDED(newMesh->Create(vertices, indices)))
            {
                m_meshes.push_back(std::move(newMesh));
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

        return S_OK;
    }

    void Model::Update(float deltaTime)
    {

    }
}