#include "structures/voxel/spk_cuboid_voxel_shape.hpp"

#include <stdexcept>
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

	void validateBounds(const spk::Vector3 &p_minimum, const spk::Vector3 &p_maximum)
	{
		if (p_minimum.x < 0.0f || p_minimum.y < 0.0f || p_minimum.z < 0.0f ||
			p_maximum.x > 1.0f || p_maximum.y > 1.0f || p_maximum.z > 1.0f ||
			p_minimum.x >= p_maximum.x || p_minimum.y >= p_maximum.y || p_minimum.z >= p_maximum.z)
		{
			throw std::invalid_argument("Cuboid bounds must define a positive box inside the unit voxel cell");
		}
	}
}

namespace spk
{
	CuboidVoxelShape::CuboidVoxelShape(
		TextureSlots p_textures,
		const spk::Vector3 &p_minimum,
		const spk::Vector3 &p_maximum,
		const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize),
		_minimum(p_minimum),
		_maximum(p_maximum)
	{
		validateBounds(_minimum, _maximum);
	}

	CuboidVoxelShape::CuboidVoxelShape(
		const spk::AtlasCell &p_uniformTexture,
		const spk::Vector3 &p_minimum,
		const spk::Vector3 &p_maximum,
		const spk::Vector2Int &p_atlasSize) :
		CuboidVoxelShape(uniformSlots(p_uniformTexture), p_minimum, p_maximum, p_atlasSize)
	{
	}

	void CuboidVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		faces.outer(spk::VoxelAxisPlane::PositiveX).emplace(createVerticalRectangle("posX", {_maximum.x, _minimum.y, _maximum.z}, {_maximum.x, _minimum.y, _minimum.z}, {_maximum.x, _maximum.y, _minimum.z}, {_maximum.x, _maximum.y, _maximum.z}));
		faces.outer(spk::VoxelAxisPlane::NegativeX).emplace(createVerticalRectangle("negX", {_minimum.x, _minimum.y, _minimum.z}, {_minimum.x, _minimum.y, _maximum.z}, {_minimum.x, _maximum.y, _maximum.z}, {_minimum.x, _maximum.y, _minimum.z}));
		faces.outer(spk::VoxelAxisPlane::PositiveY).emplace(createRectangle("top", {_minimum.x, _maximum.y, _maximum.z}, {_maximum.x, _maximum.y, _maximum.z}, {_maximum.x, _maximum.y, _minimum.z}, {_minimum.x, _maximum.y, _minimum.z}));
		faces.outer(spk::VoxelAxisPlane::NegativeY).emplace(createRectangle("bottom", {_minimum.x, _minimum.y, _minimum.z}, {_maximum.x, _minimum.y, _minimum.z}, {_maximum.x, _minimum.y, _maximum.z}, {_minimum.x, _minimum.y, _maximum.z}));
		faces.outer(spk::VoxelAxisPlane::PositiveZ).emplace(createVerticalRectangle("posZ", {_minimum.x, _minimum.y, _maximum.z}, {_maximum.x, _minimum.y, _maximum.z}, {_maximum.x, _maximum.y, _maximum.z}, {_minimum.x, _maximum.y, _maximum.z}));
		faces.outer(spk::VoxelAxisPlane::NegativeZ).emplace(createVerticalRectangle("negZ", {_maximum.x, _minimum.y, _minimum.z}, {_minimum.x, _minimum.y, _minimum.z}, {_minimum.x, _maximum.y, _minimum.z}, {_maximum.x, _maximum.y, _minimum.z}));
	}

	const spk::Vector3 &CuboidVoxelShape::minimum() const noexcept
	{
		return _minimum;
	}

	const spk::Vector3 &CuboidVoxelShape::maximum() const noexcept
	{
		return _maximum;
	}
}
