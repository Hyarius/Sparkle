#include "core/registries.hpp"

#include "core/json.hpp"
#include "world/generator/placement_rules.hpp"

#include <iostream>

namespace pg
{
	void Registries::loadAll(const std::filesystem::path &p_dataDirectory)
	{
		const std::filesystem::path gameRulesFile = p_dataDirectory / "config" / "game-rules.json";
		const spk::JSON::Value json = JsonLoader::parseFile(gameRulesFile);
		JsonReader reader(json, gameRulesFile);
		GameRules loadedGameRules = parseGameRules(reader);

		VoxelRegistry loadedVoxels;
		loadedVoxels.load(p_dataDirectory / "voxels");
		// Prefabs load before biomes: a biome's worldgen block may reference prefabs.
		Registry<PrefabDefinition> loadedPrefabs;
		loadedPrefabs.load(p_dataDirectory / "prefabs", [&loadedVoxels](JsonReader &p_reader) {
			return parsePrefabDefinition(p_reader, loadedVoxels);
		});
		Registry<BiomeDefinition> loadedBiomes;
		loadedBiomes.load(p_dataDirectory / "biomes", [&loadedVoxels, &loadedPrefabs](JsonReader &p_reader) {
			return parseBiomeDefinition(p_reader, loadedVoxels, loadedPrefabs);
		});

		const std::filesystem::path placementsFile = p_dataDirectory / "worldgen" / "placements.json";
		const spk::JSON::Value placementsJson = JsonLoader::parseFile(placementsFile);
		JsonReader placementsReader(placementsJson, placementsFile);
		PlanPlacementRules loadedPlacementRules = parsePlanPlacementRules(placementsReader, loadedPrefabs);

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_placementRules = std::move(loadedPlacementRules);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions, " << _biomes.size()
				  << " biome definitions, and " << _prefabs.size() << " prefabs" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept { return _gameRules; }
	const VoxelRegistry &Registries::voxels() const noexcept { return _voxels; }
	const Registry<BiomeDefinition> &Registries::biomes() const noexcept { return _biomes; }
	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept { return _prefabs; }
	const PlanPlacementRules &Registries::placementRules() const noexcept { return _placementRules; }
}
