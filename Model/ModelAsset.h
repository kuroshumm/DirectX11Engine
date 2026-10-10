#pragma once
#include <vector>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "SubMesh.h"

// 共有モデルアセット（ジオメトリのみ）
struct ModelAsset
{
	std::vector<SubMesh> subMeshs;
	DirectX::BoundingSphere totalLocalBounds;
};