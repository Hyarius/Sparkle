#include "structures/voxel/spk_cross_voxel_shape.hpp"

#include <utility>

namespace spk
{
	namespace
	{
		[[nodiscard]] spk::VoxelShapePolygon reversed(const spk::VoxelShapePolygon &p_polygon)
		{
			spk::VoxelShapePolygon::Builder builder;
			builder.reserve(p_polygon.size());
			for (std::size_t index = p_polygon.size(); index > 0; --index)
			{
				builder.addVertex(p_polygon[index - 1]);
			}
			return std::move(builder).bake();
		}
	}

	CrossVoxelShape::CrossVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	CrossVoxelShape::CrossVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		CrossVoxelShape(TextureSlots{{"plane", p_uniformTexture}}, p_atlasSize)
	{
	}

	void CrossVoxelShape::_constructRenderFaces()
	{
		const spk::VoxelShapePolygon alongX = createVerticalRectangle(
			"plane", {0, 0, 0.5f}, {1, 0, 0.5f}, {1, 1, 0.5f}, {0, 1, 0.5f});
		const spk::VoxelShapePolygon alongZ = createVerticalRectangle(
			"plane", {0.5f, 0, 1}, {0.5f, 0, 0}, {0.5f, 1, 0}, {0.5f, 1, 1});

		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(alongX));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(reversed(alongX)));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(alongZ));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(reversed(alongZ)));
	}
}
