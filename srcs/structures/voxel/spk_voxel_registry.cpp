#include "structures/voxel/spk_voxel_registry.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace spk
{
	std::int32_t VoxelRegistry::registerShape(std::unique_ptr<spk::VoxelShape> p_shape)
	{
		if (p_shape == nullptr)
		{
			throw std::invalid_argument("Cannot register a null voxel shape");
		}
		p_shape->initialize();
		_shapes.push_back(std::move(p_shape));
		return static_cast<std::int32_t>(_shapes.size() - 1);
	}

	const spk::VoxelShape &VoxelRegistry::shape(std::int32_t p_id) const
	{
		const spk::VoxelShape *result = tryShape(p_id);
		if (result == nullptr)
		{
			throw std::out_of_range("Unknown voxel shape id [" + std::to_string(p_id) + "]");
		}
		return *result;
	}

	const spk::VoxelShape *VoxelRegistry::tryShape(std::int32_t p_id) const noexcept
	{
		if (p_id < 0 || static_cast<std::size_t>(p_id) >= _shapes.size())
		{
			return nullptr;
		}
		return _shapes[static_cast<std::size_t>(p_id)].get();
	}

	std::size_t VoxelRegistry::size() const noexcept
	{
		return _shapes.size();
	}
}
