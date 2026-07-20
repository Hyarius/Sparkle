#include "structures/voxel/spk_hexa_plane_voxel_shape.hpp"

#include <array>
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

	HexaPlaneVoxelShape::HexaPlaneVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	HexaPlaneVoxelShape::HexaPlaneVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		HexaPlaneVoxelShape(TextureSlots{{"plane", p_uniformTexture}}, p_atlasSize)
	{
	}

	void HexaPlaneVoxelShape::_constructRenderFaces()
	{
		for (const float offset : std::array{0.2f, 0.8f})
		{
			const spk::VoxelShapePolygon xy = createVerticalRectangle(
				"plane", {0, 0, offset}, {1, 0, offset}, {1, 1, offset}, {0, 1, offset});
			const spk::VoxelShapePolygon xz = createRectangle(
				"plane", {0, offset, 0}, {0, offset, 1}, {1, offset, 1}, {1, offset, 0});
			const spk::VoxelShapePolygon yz = createVerticalRectangle(
				"plane", {offset, 0, 1}, {offset, 0, 0}, {offset, 1, 0}, {offset, 1, 1});

			for (const spk::VoxelShapePolygon *plane : {&xy, &xz, &yz})
			{
				mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(*plane));
				mutableRenderFaces().innerFaces.push_back(VoxelShapeFace(reversed(*plane)));
			}
		}
	}
}
