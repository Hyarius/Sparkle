#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"

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

	DiagonalCrossVoxelShape::DiagonalCrossVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	DiagonalCrossVoxelShape::DiagonalCrossVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		DiagonalCrossVoxelShape(TextureSlots{{"plane", p_uniformTexture}}, p_atlasSize)
	{
	}

	void DiagonalCrossVoxelShape::_constructRenderFaces()
	{
		const spk::VoxelShapePolygon first = createVerticalRectangle(
			"plane", {0, 0, 0}, {1, 0, 1}, {1, 1, 1}, {0, 1, 0});
		const spk::VoxelShapePolygon second = createVerticalRectangle(
			"plane", {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {1, 1, 0});

		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(first));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(reversed(first)));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(second));
		mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(reversed(second)));
	}
}
