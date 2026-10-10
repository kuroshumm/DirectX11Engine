#pragma once
#include <vector>
#include "Components.h"

namespace Engine
{
	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<WORD> Indices;
	};

	class GeometryGenerator
	{
	public:
		static MeshData CreateCube(float width, float height, float depth);
		static MeshData CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
		static MeshData CreatePlane(
			float width, float depth,
			uint32_t m, uint32_t n,
			float uvTilingX, float uvTilingY);
		static MeshData CreateCylinder(
			float bottomRadius, float topRadius, float height,
			uint32_t sliceCount, uint32_t stackCount);
	};
}