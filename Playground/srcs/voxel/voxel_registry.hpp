#pragma once

#include "voxel/voxel_definition.hpp"

#include "structures/voxel/spk_voxel_registry.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	// Owns two views of the same voxel catalog, kept in lockstep by numeric id:
	//  - a spk::VoxelRegistry holding the render shapes (fed to the spk voxel map/mesher), and
	//  - a parallel table of gameplay VoxelDefinitions (traversal + walk heights) for the
	//    navigation graph.
	// A numeric id therefore addresses both the render shape and the gameplay definition.
	class VoxelRegistry
	{
	private:
		spk::VoxelRegistry _renderRegistry;
		std::vector<VoxelDefinition> _definitions;
		std::vector<std::string> _numericToString;
		std::map<std::string, std::int32_t> _stringToNumeric;

	public:
		void load(const std::filesystem::path &p_directory);

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
	};
}
