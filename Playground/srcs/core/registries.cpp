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
		Registry<BiomeDefinition> loadedBiomes;
		loadedBiomes.load(p_dataDirectory / "biomes", [&loadedVoxels](JsonReader &p_reader) {
			return parseBiomeDefinition(p_reader, loadedVoxels);
		});
		Registry<PrefabDefinition> loadedPrefabs;
		loadedPrefabs.load(p_dataDirectory / "prefabs", [&loadedVoxels](JsonReader &p_reader) {
			return parsePrefabDefinition(p_reader, loadedVoxels);
		});

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions, " << _biomes.size()
				  << " biome definitions, and " << _prefabs.size() << " prefabs" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept { return _gameRules; }
	const VoxelRegistry &Registries::voxels() const noexcept { return _voxels; }
	const Registry<BiomeDefinition> &Registries::biomes() const noexcept { return _biomes; }
	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept { return _prefabs; }
}
