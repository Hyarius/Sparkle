#include "tools/core/edit_camera.hpp"

#include "tools/widgets/viewport3d.hpp"

namespace pg::tools
{
	void EditCamera::pan(Viewport3D &p_viewport, const spk::Vector2Int &p_delta) const
	{
		p_viewport.pan({static_cast<float>(p_delta.x), static_cast<float>(p_delta.y)});
	}

	void EditCamera::rotateClockwise() noexcept
	{
		switch (_orientation)
		{
		case VoxelOrientation::PositiveZ: _orientation = VoxelOrientation::PositiveX; break;
		case VoxelOrientation::PositiveX: _orientation = VoxelOrientation::NegativeZ; break;
		case VoxelOrientation::NegativeZ: _orientation = VoxelOrientation::NegativeX; break;
		case VoxelOrientation::NegativeX: _orientation = VoxelOrientation::PositiveZ; break;
		}
	}

	void EditCamera::toggleFlip() noexcept
	{
		_flip = _flip == VoxelFlip::PositiveY ? VoxelFlip::NegativeY : VoxelFlip::PositiveY;
	}

	VoxelOrientation EditCamera::orientation() const noexcept
	{
		return _orientation;
	}

	VoxelFlip EditCamera::flip() const noexcept
	{
		return _flip;
	}
}
