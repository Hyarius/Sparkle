#include "structures/voxel/spk_slope_voxel_shape.hpp"

#include <array>
#include <utility>

namespace
{
	[[nodiscard]] spk::VoxelShape::TextureSlots uniformSlots(const spk::AtlasCell &p_texture)
	{
		return {
			{"slope", p_texture},
			{"back", p_texture},
			{"bottom", p_texture},
			{"sideRight", p_texture},
			{"sideLeft", p_texture}};
	}
}

namespace spk
{
	SlopeVoxelShape::SlopeVoxelShape(TextureSlots p_textures, const spk::Vector2Int &p_atlasSize) :
		spk::VoxelShape(std::move(p_textures), p_atlasSize)
	{
	}

	SlopeVoxelShape::SlopeVoxelShape(const spk::AtlasCell &p_uniformTexture, const spk::Vector2Int &p_atlasSize) :
		SlopeVoxelShape(uniformSlots(p_uniformTexture), p_atlasSize)
	{
	}

	void SlopeVoxelShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		const std::array<spk::Vector3, 4> slopePositions = {
			spk::Vector3{0, 0, 0}, spk::Vector3{0, 1, 1}, spk::Vector3{1, 1, 1}, spk::Vector3{1, 0, 0}};
		const std::array<spk::Vector2, 4> slopeUVs = {
			spk::Vector2{0, 1}, spk::Vector2{0, 0}, spk::Vector2{1, 0}, spk::Vector2{1, 1}};
		faces.innerFaces.push_back(createPolygon("slope", slopePositions, slopeUVs));
		faces.outer(spk::VoxelAxisPlane::PositiveZ).emplace(createVerticalRectangle("back", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(spk::VoxelAxisPlane::NegativeY).emplace(createRectangle("bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(spk::VoxelAxisPlane::PositiveX).emplace(createVerticalTriangle("sideRight", {1, 0, 1}, {1, 0, 0}, {1, 1, 1}));
		faces.outer(spk::VoxelAxisPlane::NegativeX).emplace(createVerticalTriangle("sideLeft", {0, 0, 0}, {0, 0, 1}, {0, 1, 1}));
	}
}
