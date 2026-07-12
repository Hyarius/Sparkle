#pragma once

#include "voxel/voxel_definition.hpp"

#include "structures/voxel/spk_voxel_ids.hpp"
#include "structures/voxel/spk_voxel_registry.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	// Owns two views of the same voxel catalog, kept in lockstep by id:
	//  - a spk::VoxelRegistry holding the render shapes (fed to the spk voxel map/mesher),
	//    which also records each runtime id's semantic type + state, and
	//  - a parallel table of gameplay VoxelDefinitions (traversal + tags on the type, walk
	//    heights per state) for the navigation graph.
	// pg type ids are registered in lockstep with the spk registry, so a spk::VoxelTypeId
	// indexes both the render side and the gameplay definition, and a spk::VoxelRuntimeId
	// (the value a VoxelCell stores) resolves to both through the spk state references.
	//
	// Voxel geometry is data: voxels reference a ShapeCatalog entry by name, and load()
	// instantiates one render shape per authored state from that shared geometry plus the
	// state's own texture cells. The catalog is loaded separately (see Registries::loadAll)
	// and passed in as the shape factory.
	class VoxelRegistry
	{
	private:
		spk::VoxelRegistry _renderRegistry;
		std::vector<VoxelDefinition> _definitions; // indexed by spk::VoxelTypeId
		std::vector<std::string> _typeToString;
		std::map<std::string, spk::VoxelTypeId> _stringToType;

	public:
		void load(const ShapeCatalog &p_shapes, const std::filesystem::path &p_voxelsDirectory);

		// Type-level access (semantic voxel identity).
		[[nodiscard]] const VoxelDefinition &get(const std::string &p_id) const;
		[[nodiscard]] const VoxelDefinition &get(spk::VoxelTypeId p_type) const;
		[[nodiscard]] const VoxelDefinition *tryGet(const std::string &p_id) const noexcept;
		[[nodiscard]] const VoxelDefinition *tryGet(spk::VoxelTypeId p_type) const noexcept;

		// Resolution by the dense runtime handle a VoxelCell stores.
		[[nodiscard]] const VoxelDefinition &definition(spk::VoxelRuntimeId p_runtime) const;
		[[nodiscard]] const VoxelDefinition *tryDefinition(spk::VoxelRuntimeId p_runtime) const noexcept;
		[[nodiscard]] const VoxelStateDefinition &state(spk::VoxelRuntimeId p_runtime) const;
		[[nodiscard]] const VoxelStateDefinition *tryState(spk::VoxelRuntimeId p_runtime) const noexcept;

		[[nodiscard]] spk::VoxelTypeId typeId(const std::string &p_id) const;
		[[nodiscard]] spk::VoxelRuntimeId runtimeId(const std::string &p_id, spk::VoxelStateId p_state = {}) const;
		[[nodiscard]] spk::VoxelRuntimeId runtimeId(spk::VoxelTypeId p_type, spk::VoxelStateId p_state = {}) const;

		// Compatibility alias: resolves the DEFAULT state (state 0) of the given semantic
		// voxel id, exactly like runtimeId(p_id). Prefer runtimeId() in new code.
		[[nodiscard]] spk::VoxelRuntimeId numericId(const std::string &p_id) const;

		[[nodiscard]] const std::string &stringId(spk::VoxelTypeId p_type) const;
		// Debug formatting of one runtime state: "water" (default state), "water@source",
		// "bush@1" (unnamed non-default state). Never a semantic id - do not parse it back.
		[[nodiscard]] std::string debugName(spk::VoxelRuntimeId p_runtime) const;

		// One entry per semantic voxel type (not per runtime state).
		[[nodiscard]] const std::vector<std::string> &ids() const noexcept;
		[[nodiscard]] std::size_t typeCount() const noexcept;
		[[nodiscard]] std::size_t runtimeStateCount() const noexcept;

		// The render-side registry, consumed by the spk voxel map and mesher. Fluids
		// declared through the "fluid" block live there too (spk::VoxelRegistry owns the
		// families and the runtime-id classification): see fluidFamilies() / tryFluidRef().
		[[nodiscard]] const spk::VoxelRegistry &renderRegistry() const noexcept;
	};
}
