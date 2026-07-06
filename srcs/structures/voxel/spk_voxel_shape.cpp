#include "structures/voxel/spk_voxel_shape.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	std::optional<spk::VoxelShapeFace> &VoxelShapeFaceSet::outer(spk::VoxelAxisPlane p_plane)
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	const std::optional<spk::VoxelShapeFace> &VoxelShapeFaceSet::outer(spk::VoxelAxisPlane p_plane) const
	{
		return outerShell.at(static_cast<std::size_t>(p_plane));
	}

	std::size_t VoxelShapeFaceSet::outerFaceCount() const noexcept
	{
		return static_cast<std::size_t>(std::ranges::count_if(outerShell, [](const auto &p_face) {
			return p_face.has_value();
		}));
	}

	VoxelShape::VoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		_textures(std::move(p_textures)),
		_atlasSize(p_atlasSize)
	{
		if (p_atlasSize.x <= 0 || p_atlasSize.y <= 0)
		{
			throw std::invalid_argument("Voxel shape atlas size must be strictly positive");
		}
	}

	spk::VoxelShapeFaceSet &VoxelShape::mutableRenderFaces() noexcept
	{
		return _renderFaces;
	}

	const spk::AtlasCell &VoxelShape::texture(const std::string &p_slot) const
	{
		const auto iterator = _textures.find(p_slot);
		if (iterator == _textures.end())
		{
			throw std::logic_error("Voxel shape texture slot was not provided: " + p_slot);
		}
		return iterator->second;
	}

	spk::VoxelShapePolygon VoxelShape::createPolygon(
		const std::string &p_slot,
		std::span<const spk::Vector3> p_positions,
		std::span<const spk::Vector2> p_localUVs) const
	{
		if (p_positions.size() < 3 || p_positions.size() != p_localUVs.size())
		{
			throw std::invalid_argument("Voxel shape polygon requires matching position/UV lists with at least three vertices");
		}

		spk::VoxelShapePolygon result;
		result.reserve(p_positions.size());
		for (std::size_t index = 0; index < p_positions.size(); ++index)
		{
			result.push_back({p_positions[index], atlasUV(texture(p_slot), p_localUVs[index])});
		}
		return result;
	}

	spk::VoxelShapePolygon VoxelShape::createRectangle(
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

	spk::VoxelShapePolygon VoxelShape::createVerticalRectangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c,
		const spk::Vector3 &p_d,
		float p_bottomV,
		float p_topV) const
	{
		const std::array positions = {p_a, p_b, p_c, p_d};
		const std::array<spk::Vector2, 4> uvs = {
			spk::Vector2{0.0f, p_bottomV},
			spk::Vector2{1.0f, p_bottomV},
			spk::Vector2{1.0f, p_topV},
			spk::Vector2{0.0f, p_topV}};
		return createPolygon(p_slot, positions, uvs);
	}

	spk::VoxelShapePolygon VoxelShape::createTriangle(
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

	spk::VoxelShapePolygon VoxelShape::createVerticalTriangle(
		const std::string &p_slot,
		const spk::Vector3 &p_a,
		const spk::Vector3 &p_b,
		const spk::Vector3 &p_c) const
	{
		const std::array positions = {p_a, p_b, p_c};
		const std::array<spk::Vector2, 3> uvs = {
			spk::Vector2{0.0f, 1.0f},
			spk::Vector2{1.0f, 1.0f},
			spk::Vector2{1.0f, 0.0f}};
		return createPolygon(p_slot, positions, uvs);
	}

	spk::VoxelShapeFace VoxelShape::createFace(spk::VoxelShapePolygon p_polygon)
	{
		spk::VoxelShapeFace result;
		result.polygons.push_back(std::move(p_polygon));
		return result;
	}

	void VoxelShape::initialize()
	{
		if (_initialized)
		{
			return;
		}
		_constructRenderFaces();
		_initialized = true;
	}

	bool VoxelShape::initialized() const noexcept
	{
		return _initialized;
	}

	const spk::VoxelShapeFaceSet &VoxelShape::renderFaces() const noexcept
	{
		return _renderFaces;
	}

	const spk::Vector2Int &VoxelShape::atlasSize() const noexcept
	{
		return _atlasSize;
	}

	spk::Vector2 VoxelShape::atlasUV(const spk::AtlasCell &p_cell, const spk::Vector2 &p_localUV) const
	{
		return {
			(static_cast<float>(p_cell.column) + p_localUV.x) / static_cast<float>(_atlasSize.x),
			(static_cast<float>(p_cell.row) + p_localUV.y) / static_cast<float>(_atlasSize.y)};
	}
}
