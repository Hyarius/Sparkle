#pragma once

#include "voxel/voxel_enums.hpp"

#include <sparkle.hpp>

namespace pg::tools
{
	class Viewport3D;

	class EditCamera
	{
	private:
		VoxelOrientation _orientation = VoxelOrientation::PositiveZ;
		VoxelFlip _flip = VoxelFlip::PositiveY;

	public:
		void pan(Viewport3D &p_viewport, const spk::Vector2Int &p_delta) const;
		void rotateClockwise() noexcept;
		void toggleFlip() noexcept;
		[[nodiscard]] VoxelOrientation orientation() const noexcept;
		[[nodiscard]] VoxelFlip flip() const noexcept;
	};
}
