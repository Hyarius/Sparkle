#include "voxel/shapes/stair_shape.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pg
{
	StairShape::StairShape(TextureSlots p_textures, int p_stepCount) :
		VoxelShape(std::move(p_textures)),
		_stepCount(p_stepCount)
	{
		if (_stepCount <= 0)
		{
			throw std::invalid_argument("Stair step count must be positive");
		}
	}

	VoxelShapeFace StairShape::_constructSideFace(bool p_positiveX) const
	{
		const float x = p_positiveX ? 1.0f : 0.0f;
		std::vector<spk::Vector3> positions;
		positions.reserve(static_cast<std::size_t>(2 + 2 * _stepCount));
		positions.push_back({x, 0, 1});
		positions.push_back({x, 0, 0});

		for (int step = 0; step < _stepCount; ++step)
		{
			const float z0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float z1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			const float y = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			positions.push_back({x, y, z0});
			positions.push_back({x, y, z1});
		}

		if (!p_positiveX)
		{
			std::ranges::reverse(positions);
		}

		std::vector<spk::Vector2> uvs;
		uvs.reserve(positions.size());
		for (const spk::Vector3 &position : positions)
		{
			uvs.push_back({position.z, position.y});
		}

		return createFace(createPolygon(
			p_positiveX ? "sideRight" : "sideLeft",
			positions,
			uvs));
	}

	void StairShape::_constructRenderFaces()
	{
		auto &faces = mutableRenderFaces();
		for (int step = 0; step < _stepCount; ++step)
		{
			const float z0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float z1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);
			const float y0 = static_cast<float>(step) / static_cast<float>(_stepCount);
			const float y1 = static_cast<float>(step + 1) / static_cast<float>(_stepCount);

			faces.innerFaces.push_back(createFace(createRectangle(
				"top", {0, y1, z1}, {1, y1, z1}, {1, y1, z0}, {0, y1, z0})));
			faces.innerFaces.push_back(createFace(createRectangle(
				"riser", {1, y0, z0}, {0, y0, z0}, {0, y1, z0}, {1, y1, z0})));
		}

		faces.outer(VoxelAxisPlane::PositiveZ) = createFace(createRectangle(
			"back", {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}));
		faces.outer(VoxelAxisPlane::NegativeY) = createFace(createRectangle(
			"bottom", {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}));
		faces.outer(VoxelAxisPlane::PositiveX) = _constructSideFace(true);
		faces.outer(VoxelAxisPlane::NegativeX) = _constructSideFace(false);
	}

	void StairShape::_constructMask()
	{
		for (int step = 0; step < _stepCount; ++step)
		{
			mutableMaskFaces().positiveY.push_back(renderFaces().innerFaces[static_cast<std::size_t>(step * 2)]);
		}
		mutableMaskFaces().negativeY.push_back(*renderFaces().outer(VoxelAxisPlane::NegativeY));
	}

	CardinalHeightSet StairShape::_constructPositiveYHeights() const
	{
		const float firstStepMidpoint = 1.0f / (2.0f * static_cast<float>(_stepCount));
		return {0.5f, 0.5f, 1.0f, firstStepMidpoint, 0.5f};
	}

	CardinalHeightSet StairShape::_constructNegativeYHeights() const
	{
		return {1, 1, 1, 1, 1};
	}

	int StairShape::stepCount() const noexcept
	{
		return _stepCount;
	}
}
