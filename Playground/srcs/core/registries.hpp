#pragma once

#include "core/game_rules.hpp"
#include "voxel/voxel_registry.hpp"

#include <filesystem>

namespace pg
{
	class Registries
	{
	private:
		GameRules _gameRules;
		VoxelRegistry _voxels;

	public:
		void loadAll(const std::filesystem::path &p_dataDirectory);

		[[nodiscard]] const GameRules &gameRules() const noexcept;
		[[nodiscard]] const VoxelRegistry &voxels() const noexcept;
	};
}
