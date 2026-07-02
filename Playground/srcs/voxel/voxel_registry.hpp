#pragma once

#include "core/registry.hpp"
#include "voxel/voxel_definition.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	class VoxelRegistry
	{
	private:
		Registry<VoxelDefinition> _definitions;
		std::vector<std::string> _numericToString;
		std::vector<const VoxelDefinition *> _numericDefinitions;
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
	};
}
