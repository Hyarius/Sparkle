#include "structures/voxel/spk_voxel_registry.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace spk
{
	spk::VoxelRuntimeId VoxelTypeRegistration::defaultRuntimeId() const
	{
		if (states.empty())
		{
			throw std::logic_error("Voxel type registration holds no state");
		}
		return states.front();
	}

	VoxelTypeRegistration VoxelRegistry::registerType(std::vector<std::unique_ptr<spk::VoxelShape>> p_states)
	{
		if (p_states.empty())
		{
			throw std::invalid_argument("Cannot register a voxel type without any state");
		}
		for (const std::unique_ptr<spk::VoxelShape> &shape : p_states)
		{
			if (shape == nullptr)
			{
				throw std::invalid_argument("Cannot register a null voxel shape");
			}
		}
		if (_types.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
		{
			throw std::length_error("Voxel registry exceeds type id capacity");
		}
		if (p_states.size() > static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1)
		{
			throw std::length_error("Voxel type exceeds state id capacity");
		}
		if (p_states.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) - _shapes.size())
		{
			throw std::length_error("Voxel registry exceeds runtime id capacity");
		}

		// Initialize every shape before mutating any storage: a throwing shape must leave
		// the registry untouched (no partial type, no mismatched mapping vectors).
		for (const std::unique_ptr<spk::VoxelShape> &shape : p_states)
		{
			shape->initialize();
		}

		const spk::VoxelTypeId type{static_cast<std::uint32_t>(_types.size())};
		VoxelTypeRegistration registration{.type = type};
		VoxelTypeRecord record;

		// Reserve everything up front so the appends below cannot throw mid-way.
		registration.states.reserve(p_states.size());
		record.states.reserve(p_states.size());
		_shapes.reserve(_shapes.size() + p_states.size());
		_runtimeToState.reserve(_runtimeToState.size() + p_states.size());
		_types.reserve(_types.size() + 1);

		for (std::size_t stateIndex = 0; stateIndex < p_states.size(); ++stateIndex)
		{
			const spk::VoxelRuntimeId runtime{static_cast<std::int32_t>(_shapes.size())};
			_shapes.push_back(std::move(p_states[stateIndex]));
			_runtimeToState.push_back({.type = type, .state = {static_cast<std::uint16_t>(stateIndex)}});
			record.states.push_back(runtime);
			registration.states.push_back(runtime);
		}
		_types.push_back(std::move(record));
		return registration;
	}

	spk::VoxelRuntimeId VoxelRegistry::registerShape(std::unique_ptr<spk::VoxelShape> p_shape)
	{
		std::vector<std::unique_ptr<spk::VoxelShape>> states;
		states.push_back(std::move(p_shape));
		return registerType(std::move(states)).defaultRuntimeId();
	}

	const spk::VoxelShape &VoxelRegistry::shape(spk::VoxelRuntimeId p_runtimeId) const
	{
		const spk::VoxelShape *result = tryShape(p_runtimeId);
		if (result == nullptr)
		{
			throw std::out_of_range("Unknown voxel runtime id [" + std::to_string(p_runtimeId.value) + "]");
		}
		return *result;
	}

	const spk::VoxelShape *VoxelRegistry::tryShape(spk::VoxelRuntimeId p_runtimeId) const noexcept
	{
		if (!p_runtimeId.isValid() || p_runtimeId.index() >= _shapes.size())
		{
			return nullptr;
		}
		return _shapes[p_runtimeId.index()].get();
	}

	const spk::VoxelStateReference &VoxelRegistry::stateReference(spk::VoxelRuntimeId p_runtimeId) const
	{
		const spk::VoxelStateReference *result = tryStateReference(p_runtimeId);
		if (result == nullptr)
		{
			throw std::out_of_range("Unknown voxel runtime id [" + std::to_string(p_runtimeId.value) + "]");
		}
		return *result;
	}

	const spk::VoxelStateReference *VoxelRegistry::tryStateReference(spk::VoxelRuntimeId p_runtimeId) const noexcept
	{
		if (!p_runtimeId.isValid() || p_runtimeId.index() >= _runtimeToState.size())
		{
			return nullptr;
		}
		return &_runtimeToState[p_runtimeId.index()];
	}

	spk::VoxelRuntimeId VoxelRegistry::runtimeId(spk::VoxelTypeId p_type, spk::VoxelStateId p_state) const
	{
		const std::optional<spk::VoxelRuntimeId> result = tryRuntimeId(p_type, p_state);
		if (!result.has_value())
		{
			throw std::out_of_range(
				"Unknown voxel type [" + std::to_string(p_type.value) + "] / state [" +
				std::to_string(p_state.value) + "]");
		}
		return *result;
	}

	std::optional<spk::VoxelRuntimeId> VoxelRegistry::tryRuntimeId(
		spk::VoxelTypeId p_type, spk::VoxelStateId p_state) const noexcept
	{
		if (p_type.index() >= _types.size())
		{
			return std::nullopt;
		}
		const VoxelTypeRecord &record = _types[p_type.index()];
		if (p_state.index() >= record.states.size())
		{
			return std::nullopt;
		}
		return record.states[p_state.index()];
	}

	std::size_t VoxelRegistry::typeCount() const noexcept
	{
		return _types.size();
	}

	std::size_t VoxelRegistry::runtimeStateCount() const noexcept
	{
		return _shapes.size();
	}

	std::size_t VoxelRegistry::stateCount(spk::VoxelTypeId p_type) const
	{
		if (p_type.index() >= _types.size())
		{
			throw std::out_of_range("Unknown voxel type [" + std::to_string(p_type.value) + "]");
		}
		return _types[p_type.index()].states.size();
	}

	std::size_t VoxelRegistry::size() const noexcept
	{
		return runtimeStateCount();
	}
}
