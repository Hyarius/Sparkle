#pragma once

#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_shape.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace spk
{
	// Result of registering one semantic voxel type: the attributed type id plus the dense
	// runtime id of each state, indexed by state id (states[k] is state k's handle).
	struct VoxelTypeRegistration
	{
		spk::VoxelTypeId type;
		std::vector<spk::VoxelRuntimeId> states;

		// State 0 is the default state of every type.
		[[nodiscard]] spk::VoxelRuntimeId defaultRuntimeId() const;
	};

	// Owns the render shapes of every registered voxel state and maintains the mappings
	// between the three voxel identities:
	//   VoxelRuntimeId -> VoxelShape                    (hot path, used by the mesher)
	//   VoxelRuntimeId -> VoxelTypeId + VoxelStateId
	//   VoxelTypeId + VoxelStateId -> VoxelRuntimeId
	// Shapes are initialized on registration. Type ids, state ids and runtime ids are all
	// dense and attributed sequentially from 0; state ids of one type are contiguous from 0
	// and state 0 is the default state, so state lookup is direct indexing (no hashing).
	class VoxelRegistry
	{
	private:
		struct VoxelTypeRecord
		{
			std::vector<spk::VoxelRuntimeId> states;
		};

		std::vector<std::unique_ptr<spk::VoxelShape>> _shapes;
		std::vector<spk::VoxelStateReference> _runtimeToState;
		std::vector<VoxelTypeRecord> _types;

	public:
		// Registers one semantic voxel type whose states are the given shapes: p_states[k]
		// becomes state k (at least one state is required, state 0 is the default). Every
		// shape is validated and initialized before any registry storage mutates, so a
		// failure never leaves a partial type or mismatched mapping vectors.
		[[nodiscard]] VoxelTypeRegistration registerType(std::vector<std::unique_ptr<spk::VoxelShape>> p_states);

		// Legacy single-shape registration: creates a new type with a single state 0
		// through registerType() and returns its runtime id. New code that cares about
		// the semantic type should call registerType() directly.
		spk::VoxelRuntimeId registerShape(std::unique_ptr<spk::VoxelShape> p_shape);

		[[nodiscard]] const spk::VoxelShape &shape(spk::VoxelRuntimeId p_runtimeId) const;
		[[nodiscard]] const spk::VoxelShape *tryShape(spk::VoxelRuntimeId p_runtimeId) const noexcept;

		[[nodiscard]] const spk::VoxelStateReference &stateReference(spk::VoxelRuntimeId p_runtimeId) const;
		[[nodiscard]] const spk::VoxelStateReference *tryStateReference(spk::VoxelRuntimeId p_runtimeId) const noexcept;

		[[nodiscard]] spk::VoxelRuntimeId runtimeId(spk::VoxelTypeId p_type, spk::VoxelStateId p_state) const;
		[[nodiscard]] std::optional<spk::VoxelRuntimeId> tryRuntimeId(
			spk::VoxelTypeId p_type, spk::VoxelStateId p_state) const noexcept;

		// Number of registered semantic types.
		[[nodiscard]] std::size_t typeCount() const noexcept;
		// Number of registered runtime states (== number of render shapes).
		[[nodiscard]] std::size_t runtimeStateCount() const noexcept;
		// Number of states of one type.
		[[nodiscard]] std::size_t stateCount(spk::VoxelTypeId p_type) const;

		// Compatibility alias: the number of registered runtime states, exactly like
		// runtimeStateCount() - NOT the number of semantic types.
		[[nodiscard]] std::size_t size() const noexcept;
	};
}
