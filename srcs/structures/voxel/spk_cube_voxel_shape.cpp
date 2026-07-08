#include "structures/voxel/spk_cube_voxel_shape.hpp"

#include <utility>

namespace
{
	[[nodiscard]] spk::VoxelShape::TextureSlots uniformSlots(const spk::AtlasCell &p_texture)
	{
		return {
			{"posX", p_texture},
			{"negX", p_texture},
			{"top", p_texture},
			{"bottom", p_texture},
			{"posZ", p_texture},
			{"negZ", p_texture}};
	}
}

namespace spk
{
	CubeVoxelShape::CubeVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	CubeVoxelShape::CubeVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(uniformSlots(p_uniformTexture), p_atlasSize)
	{
	}

	void CubeVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.outer(spk::VoxelAxisPlane::PositiveX).emplace(createVerticalRectangle("posX", {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}));
		faces.outer(spk::VoxelAxisPlane::NegativeX).emplace(createVerticalRectangle("negX", {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}));
		faces.outer(spk::VoxelAxisPlane::PositiveY).emplace(createRectangle("top", {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}));
		faces.outer(spk::VoxelAxisPlane::NegativeY).emplace(createRectangle("bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(spk::VoxelAxisPlane::PositiveZ).emplace(createVerticalRectangle("posZ", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(spk::VoxelAxisPlane::NegativeZ).emplace(createVerticalRectangle("negZ", {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}));
	}
}
