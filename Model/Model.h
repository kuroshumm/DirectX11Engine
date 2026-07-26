#pragma once
#include <vector>
#include <string>
#include <memory>
#include <DirectXCollision.h>
#include "Graphics.h"
#include "Mesh.h"
#include "Material.h"
#include "IModel.h"

namespace Engine
{
    // 外部の3Dアセットファイルを抽象化し、複数のメッシュリソースを統括するクラス
    class Model : public IModel
    {
    public:
        Model(Graphics* pGfx);
        ~Model();

        // 指定されたファイルパスからジオメトリデータを抽出し、VRAMへロードする
        HRESULT LoadMesh(const std::string& filePath) override;
		void SetMaterial(const std::shared_ptr<Material>& pMaterial) override;
        // 保持している全てのメッシュストリームに対するドローコールを発行する
        void Draw() override;
        void Update(float deltaTime) override;

        
        DirectX::BoundingSphere GetLocalBoundingSphere() const override { return m_LocalBoundingSphere; }

    private:
        Graphics* m_pGfx;

		// ローカル座標系における境界球 (モデル全体を包み込む最小の球体)
        DirectX::BoundingSphere m_LocalBoundingSphere;
		// モデルは複数のメッシュを持つことができるため、Meshオブジェクトのベクタで管理する
        std::vector<std::unique_ptr<Mesh>> m_meshes;
		// モデル全体で共有されるマテリアル (シェーダーやテクスチャなどのレンダリングリソース) を保持
		std::shared_ptr<Material> m_pMaterial;
    };
}