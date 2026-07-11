#pragma once

#include "voxel/fluid.hpp"
#include "voxel/voxel_definition.hpp"

#include "structures/voxel/spk_voxel_registry.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace pg
{
	// Owns two views of the same voxel catalog, kept in lockstep by numeric id:
	//  - a spk::VoxelRegistry holding the render shapes (fed to the spk voxel map/mesher), and
	//  - a parallel table of gameplay VoxelDefinitions (traversal + walk heights) for the
	//    navigation graph.
	// A numeric id therefore addresses both the render shape and the gameplay definition.
	//
	// Voxel geometry is data: voxels reference a ShapeCatalog entry by name, and load()
	// instantiates one render shape per voxel from that shared geometry plus the voxel's
	// own texture cells. The catalog is loaded separately (see Registries::loadAll) and
	// passed in as the shape factory.
	class VoxelRegistry
	{
	private:
		spk::VoxelRegistry _renderRegistry;
		std::vector<VoxelDefinition> _definitions;
		std::vector<std::string> _numericToString;
		std::map<std::string, std::int32_t> _stringToNumeric;
		// Fluids and, for every fluid cell id (source or generated stage), where it sits in its
		// family - so the simulation can classify a cell in O(1).
		std::vector<FluidFamily> _fluids;
		std::unordered_map<std::int32_t, FluidRef> _fluidRefs;

	public:
		void load(const ShapeCatalog &p_shapes, const std::filesystem::path &p_voxelsDirectory);

		[[nodiscard]] const VoxelDefinition &get(const std::string &p_id) const;
		[[nodiscard]] const VoxelDefinition &get(std::int32_t p_id) const;
		[[nodiscard]] const VoxelDefinition *tryGet(const std::string &p_id) const noexcept;
		[[nodiscard]] const VoxelDefinition *tryGet(std::int32_t p_id) const noexcept;
		[[nodiscard]] std::int32_t numericId(const std::string &p_id) const;
		[[nodiscard]] const std::string &stringId(std::int32_t p_id) const;
		[[nodiscard]] const std::vector<std::string> &ids() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;

		// The render-side registry, consumed by the spk voxel map and mesher.
		[[nodiscard]] const spk::VoxelRegistry &renderRegistry() const noexcept;

		// Fluids declared through the "fluid" shape type, resolved to numeric ids.
		[[nodiscard]] const std::vector<FluidFamily> &fluids() const noexcept;
		// Classifies a cell id as a fluid source / stage, or nullptr if it is not a fluid.
		[[nodiscard]] const FluidRef *tryFluidRef(std::int32_t p_id) const noexcept;
	};
}
