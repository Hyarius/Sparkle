#include "voxel/voxel_shape.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pg
{
	std::optional<VoxelShapeFace> &VoxelShapeFaceSet::outer(VoxelAxisPlane p_plane)
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	const std::optional<VoxelShapeFace> &VoxelShapeFaceSet::outer(VoxelAxisPlane p_plane) const
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	std::size_t VoxelShapeFaceSet::outerFaceCount() const noexcept
	{
		return static_cast<std::size_t>(std::ranges::count_if(outerShell, [](const auto &p_face) {
			return p_face.has_value();
		}));
	}

	const std::vector<VoxelShapeFace> &VoxelShapeMaskSet::faces(VoxelFlip p_flip) const noexcept
	{
		return p_flip == VoxelFlip::PositiveY ? positiveY : negativeY;
	}

	float CardinalHeightSet::get(VoxelOrientation p_direction) const
	{
		switch (p_direction)
		{
		case VoxelOrientation::PositiveX:
			return positiveX;
		case VoxelOrientation::PositiveZ:
			return positiveZ;
		case VoxelOrientation::NegativeX:
			return negativeX;
		case VoxelOrientation::NegativeZ:
			return negativeZ;
		}
		throw std::invalid_argument("Unknown voxel direction");
	}

	const CardinalHeightSet &CardinalHeightCollection::get(VoxelFlip p_flip) const noexcept
	{
		return p_flip == VoxelFlip::PositiveY ? positiveY : negativeY;
	}

	VoxelShape::VoxelShape(TextureSlots p_textures) :
		_textures(std::move(p_textures))
	{
	}

	VoxelShapeFaceSet &VoxelShape::mutableRenderFaces() noexcept
	{
		return _renderFaces;
	}

	VoxelShapeMaskSet &VoxelShape::mutableMaskFaces() noexcept
	{
		return _maskFaces;
	}

	const AtlasCell &VoxelShape::texture(const std::string &p_slot) const
	{
		const auto iterator = _textures.find(p_slot);
		if (iterator == _textures.end())
		{
			throw std::logic_error("Voxel shape texture slot was not provided: " + p_slot);
		}
		return iterator->second;
	}

	VoxelShapePolygon VoxelShape::createPolygon(
		const std::string &p_slot,
		std::span<const spk::Vector3> p_positions,
		std::span<const spk::Vector2> p_localUVs) const
	{
		if (p_positions.size() < 3 || p_positions.size() != p_localUVs.size())
		{
			throw std::invalid_argument("Voxel shape polygon requires matching position/UV lists with at least three vertices");
		}

		VoxelShapePolygon result;
		result.reserve(p_positions.size());
		for (std::size_t index = 0; index < p_positions.size(); ++index)
		{
			result.push_back({p_positions[index], atlasUV(texture(p_slot), p_localUVs[index])});
		}
		return result;
	}

	VoxelShapePolygon VoxelShape::createRectangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c,
		const spk::Vector3 &p_d) const
	{
		const std::array positions = {p_a, p_b, p_c, p_d};
		const std::array<spk::Vector2, 4> uvs = {
			spk::Vector2{0.0f, 0.0f},
			spk::Vector2{1.0f, 0.0f},
			spk::Vector2{1.0f, 1.0f},
			spk::Vector2{0.0f, 1.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	VoxelShapePolygon VoxelShape::createTriangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c) const
	{
		const std::array positions = {p_a, p_b, p_c};
		const std::array<spk::Vector2, 3> uvs = {
			spk::Vector2{0.0f, 0.0f},
			spk::Vector2{1.0f, 0.0f},
			spk::Vector2{1.0f, 1.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	VoxelShapeFace VoxelShape::createFace(VoxelShapePolygon p_polygon)
	{
		VoxelShapeFace result;
		result.polygons.push_back(std::move(p_polygon));
		return result;
	}

	CardinalHeightSet VoxelShape::_constructNegativeYHeights() const
	{
		return _constructPositiveYHeights();
	}

	void VoxelShape::initialize()
	{
		if (_initialized)
		{
			return;
		}
		_constructRenderFaces();
		_constructMask();
		_cardinalHeights.positiveY = _constructPositiveYHeights();
		_cardinalHeights.negativeY = _constructNegativeYHeights();
		_initialized = true;
	}

	bool VoxelShape::initialized() const noexcept
	{
		return _initialized;
	}

	const VoxelShapeFaceSet &VoxelShape::renderFaces() const noexcept
	{
		return _renderFaces;
	}

	const VoxelShapeMaskSet &VoxelShape::maskFaces() const noexcept
	{
		return _maskFaces;
	}

	const CardinalHeightSet &VoxelShape::heights(VoxelFlip p_flip) const noexcept
	{
		return _cardinalHeights.get(p_flip);
	}

	spk::Vector2 VoxelShape::atlasUV(const AtlasCell &p_cell, const spk::Vector2 &p_localUV)
	{
		return {
			(static_cast<float>(p_cell.column) + p_localUV.x) / static_cast<float>(AtlasColumns),
			(static_cast<float>(p_cell.row) + p_localUV.y) / static_cast<float>(AtlasRows)};
	}
}
