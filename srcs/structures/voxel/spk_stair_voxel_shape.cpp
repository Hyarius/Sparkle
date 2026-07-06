#include "structures/voxel/spk_stair_voxel_shape.hpp"

#include <stdexcept>
#include <utility>

namespace
{
	[[nodiscard]] spk::VoxelShape::TextureSlots uniformSlots(const spk::AtlasCell &p_texture)
	{
		return {
			{"top", p_texture},
			{"riser", p_texture},
			{"back", p_texture},
			{"bottom", p_texture},
			{"sideRight", p_texture},
			{"sideLeft", p_texture}};
	}
}

namespace spk
{
	StairVoxelShape::StairVoxelShape(TextureSlots p_textures, int p_stepCount, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize),
		_stepCount(p_stepCount)
	{
		if (_stepCount <= 0)
		{
			throw std::invalid_argument("Stair step count must be positive");
		}
	}

	StairVoxelShape::StairVoxelShape(const spk::AtlasCell &p_uniformTexture, int p_stepCount, const spk::Vector2Int &p_atlasSize) :
		StairVoxelShape(uniformSlots(p_uniformTexture), p_stepCount, p_atlasSize)
	{
	}

	spk::VoxelShapeFace StairVoxelShape::_constructSideFace(bool p_positiveX) const
	{
		const float x = p_positiveX ? 1.0f : 0.0f;
		spk::VoxelShapeFace face;
		face.polygons.reserve(static_cast<std::size_t>(_stepCount));
		for (int step = 0; step < _stepCount; ++step)
		{
			const float z0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float z1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			const float y = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			const std::array<spk::Vector3, 4> positions = p_positiveX
															  ? std::array<spk::Vector3, 4>{{{x, 0, z1}, {x, 0, z0}, {x, y, z0}, {x, y, z1}}}
															  : std::array<spk::Vector3, 4>{{{x, 0, z0}, {x, 0, z1}, {x, y, z1}, {x, y, z0}}};
			std::array<spk::Vector2, 4> uvs;
			for (std::size_t index = 0; index < positions.size(); ++index)
			{
				uvs[index] = {positions[index].z, 1.0f - positions[index].y};
			}
			face.polygons.push_back(createPolygon(
				p_positiveX ? "sideRight" : "sideLeft",
				positions,
				uvs));
		}
		return face;
	}

	void StairVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		for (int step = 0; step < _stepCount; ++step)
		{
			const float z0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float z1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			const float y0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float y1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);

			const std::array<spk::Vector3, 4> topPositions = {
				spk::Vector3{0, y1, z1}, spk::Vector3{1, y1, z1}, spk::Vector3{1, y1, z0}, spk::Vector3{0, y1, z0}};
			const std::array<spk::Vector2, 4> topUVs = {
				spk::Vector2{0, 1.0f - z1},
				spk::Vector2{1, 1.0f - z1},
				spk::Vector2{1, 1.0f - z0},
				spk::Vector2{0, 1.0f - z0}};
			faces.innerFaces.push_back(createFace(createPolygon("top", topPositions, topUVs)));
			faces.innerFaces.push_back(createFace(createVerticalRectangle(
				"riser", {1, y0, z0}, {0, y0, z0}, {0, y1, z0}, {1, y1, z0}, 1.0f - y0, 1.0f - y1)));
		}

		faces.outer(spk::VoxelAxisPlane::PositiveZ) = createFace(createVerticalRectangle(
			"back", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(spk::VoxelAxisPlane::NegativeY) = createFace(createRectangle(
			"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(spk::VoxelAxisPlane::PositiveX) = _constructSideFace(true);
		faces.outer(spk::VoxelAxisPlane::NegativeX) = _constructSideFace(false);
	}

	int StairVoxelShape::stepCount() const noexcept
	{
		return _stepCount;
	}
}
