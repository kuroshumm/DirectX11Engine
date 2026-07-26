#pragma once
#include <wrl/client.h>
#include <string>
#include <memory>
#include <DirectXCollision.h>

namespace Engine
{
	class Material;

	class IModel
	{
	public:
		virtual ~IModel() = default;

		virtual HRESULT LoadMesh(const std::string& filePath) = 0;
		virtual void SetMaterial(const std::shared_ptr<Material>& pMaterial) = 0;
		virtual void Draw() = 0;
		virtual void Update(float deltaTime) {}

		virtual DirectX::BoundingSphere GetLocalBoundingSphere() const = 0;
	};
}