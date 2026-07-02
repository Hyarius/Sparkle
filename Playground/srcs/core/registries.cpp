#include "core/registries.hpp"

#include "core/json.hpp"

#include <iostream>

namespace pg
{
	void Registries::loadAll(const std::filesystem::path &p_dataDirectory)
	{
		const std::filesystem::path gameRulesFile = p_dataDirectory / "config" / "game-rules.json";
		nlohmann::json json = JsonLoader::parseFile(gameRulesFile);
		JsonReader reader(json, gameRulesFile);
		GameRules loadedGameRules = parseGameRules(reader);
		VoxelRegistry loadedVoxels;
		loadedVoxels.load(p_dataDirectory / "voxels");

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept
	{
		return _gameRules;
	}

	const VoxelRegistry &Registries::voxels() const noexcept
	{
		return _voxels;
	}
}
