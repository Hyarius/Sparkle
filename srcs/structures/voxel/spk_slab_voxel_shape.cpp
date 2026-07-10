#include "structures/voxel/spk_slab_voxel_shape.hpp"

#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] spk::VoxelShape::TextureSlots uniformSlots(const spk::AtlasCell &p_texture)
	{
		return {
			{"top", p_texture},
			{"bottom", p_texture},
			{"posX", p_texture},
			{"negX", p_texture},
			{"posZ", p_texture},
			{"negZ", p_texture}};
	}
}

namespace spk
{
	SlabVoxelShape::SlabVoxelShape(TextureSlots p_textures, float p_height, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize),
		_height(p_height)
	{
		if (_height < 0.0f || _height > 1.0f)
		{
			throw std::invalid_argument("Slab height must be between 0 and 1");
		}
	}

	SlabVoxelShape::SlabVoxelShape(const spk::AtlasCell &p_uniformTexture, float p_height, const spk::Vector2Int &p_atlasSize) :
		SlabVoxelShape(uniformSlots(p_uniformTexture), p_height, p_atlasSize)
	{
	}

	void SlabVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.outer(spk::VoxelAxisPlane::PositiveY).emplace(createRectangle("top", {0, _height, 1}, {1, _height, 1}, {1, _height, 0}, {0, _height, 0}));
		faces.outer(spk::VoxelAxisPlane::NegativeY).emplace(createRectangle("bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));

		// The sides sit on the cell boundary, so they belong to the outer shell: neighbors can
		// occlude them (a covering cube, or the same-transparency coverage rule for fluids)
		// instead of them being drawn unconditionally as inner faces.
		faces.outer(spk::VoxelAxisPlane::PositiveX).emplace(createVerticalRectangle(
			"posX", {1, 0, 1}, {1, 0, 0}, {1, _height, 0}, {1, _height, 1}, 1.0f, 1.0f - _height));
		faces.outer(spk::VoxelAxisPlane::NegativeX).emplace(createVerticalRectangle(
			"negX", {0, 0, 0}, {0, 0, 1}, {0, _height, 1}, {0, _height, 0}, 1.0f, 1.0f - _height));
		faces.outer(spk::VoxelAxisPlane::PositiveZ).emplace(createVerticalRectangle(
			"posZ", {0, 0, 1}, {1, 0, 1}, {1, _height, 1}, {0, _height, 1}, 1.0f, 1.0f - _height));
		faces.outer(spk::VoxelAxisPlane::NegativeZ).emplace(createVerticalRectangle(
			"negZ", {1, 0, 0}, {0, 0, 0}, {0, _height, 0}, {1, _height, 0}, 1.0f, 1.0f - _height));
	}

	float SlabVoxelShape::height() const noexcept
	{
		return _height;
	}
}
