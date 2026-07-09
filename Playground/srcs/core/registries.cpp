#include "core/registries.hpp"

#include "core/json.hpp"
#include "world/generator/climb_prefabs.hpp"
#include "world/generator/placement_rules.hpp"

#include <iostream>
#include <stdexcept>

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
		// Interiors reference room prefabs, so they load after prefabs; each prefab's
		// own "interior" link is validated once both registries exist.
		Registry<InteriorDefinition> loadedInteriors;
		loadedInteriors.load(p_dataDirectory / "interiors", [&loadedPrefabs](JsonReader &p_reader) {
			return parseInteriorDefinition(p_reader, loadedPrefabs);
		});
		for (const std::string &prefabId : loadedPrefabs.ids())
		{
			const PrefabDefinition &prefab = loadedPrefabs.get(prefabId);
			if (!prefab.interiorId.empty() && loadedInteriors.tryGet(prefab.interiorId) == nullptr)
			{
				throw std::runtime_error(
					"prefab '" + prefabId + "' links unknown interior '" + prefab.interiorId + "'");
			}
			if (!prefab.interiorId.empty() && prefab.tryAnchor("door") == nullptr)
			{
				throw std::runtime_error("prefab '" + prefabId + "' has an interior but no 'door' anchor");
			}
		}

		// Staircases/slopes are generated from each biome's palette voxels rather than
		// authored as prefab files: this adds the flight/platform prefabs to the registry
		// and records their ids on the biomes for the world generator to place.
		synthesizeClimbPrefabs(loadedPrefabs, loadedBiomes, loadedVoxels);

		const std::filesystem::path placementsFile = p_dataDirectory / "worldgen" / "placements.json";
		const spk::JSON::Value placementsJson = JsonLoader::parseFile(placementsFile);
		JsonReader placementsReader(placementsJson, placementsFile);
		PlanPlacementRules loadedPlacementRules = parsePlanPlacementRules(placementsReader, loadedPrefabs);

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_interiors = std::move(loadedInteriors);
		_placementRules = std::move(loadedPlacementRules);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions, " << _biomes.size()
				  << " biome definitions, " << _prefabs.size() << " prefabs, and " << _interiors.size()
				  << " interiors" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept { return _gameRules; }
	const VoxelRegistry &Registries::voxels() const noexcept { return _voxels; }
	const Registry<BiomeDefinition> &Registries::biomes() const noexcept { return _biomes; }
	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept { return _prefabs; }
	const Registry<InteriorDefinition> &Registries::interiors() const noexcept { return _interiors; }
	const PlanPlacementRules &Registries::placementRules() const noexcept { return _placementRules; }
}
