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
		Registry<Ability> loadedAbilities;
		loadedAbilities.load(p_dataDirectory / "abilities", parseAbility);
		Registry<EncounterTable> loadedEncounterTables;
		loadedEncounterTables.load(p_dataDirectory / "encounter-tables", parseEncounterTable);
		Registry<BiomeDefinition> loadedBiomes;
		loadedBiomes.load(
			p_dataDirectory / "biomes",
			[&loadedEncounterTables, &loadedVoxels](JsonReader &p_reader) {
				return parseBiomeDefinition(p_reader, loadedEncounterTables, loadedVoxels);
			});
		Registry<PrefabDefinition> loadedPrefabs;
		loadedPrefabs.load(p_dataDirectory / "prefabs", [&loadedVoxels](JsonReader &p_reader) {
			return parsePrefabDefinition(p_reader, loadedVoxels);
		});
		Registry<MapDefinition> loadedMaps;
		loadedMaps.load(p_dataDirectory / "maps", [&loadedVoxels, &loadedPrefabs](JsonReader &p_reader) {
			return parseMapDefinition(p_reader, loadedVoxels, loadedPrefabs);
		});
		for (const std::string &mapId : loadedMaps.ids())
		{
			const MapDefinition &map = loadedMaps.get(mapId);
			if (!map.biome.empty() && loadedBiomes.tryGet(map.biome) == nullptr)
			{
				throw JsonError(
					p_dataDirectory / "maps" / (mapId + ".json"),
					"$.biome",
					"unknown biome id '" + map.biome + "'");
			}
		}

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		_abilities = std::move(loadedAbilities);
		_encounterTables = std::move(loadedEncounterTables);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_maps = std::move(loadedMaps);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions" << std::endl;
		std::cout << "Loaded " << _abilities.size() << " ability definitions" << std::endl;
		std::cout << "Loaded " << _encounterTables.size() << " encounter tables and "
				  << _biomes.size() << " biome definitions" << std::endl;
		std::cout << "Loaded " << _prefabs.size() << " prefab definitions and "
				  << _maps.size() << " map definitions" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept
	{
		return _gameRules;
	}

	const VoxelRegistry &Registries::voxels() const noexcept
	{
		return _voxels;
	}

	const Registry<Ability> &Registries::abilities() const noexcept
	{
		return _abilities;
	}

	const Registry<EncounterTable> &Registries::encounterTables() const noexcept
	{
		return _encounterTables;
	}

	const Registry<BiomeDefinition> &Registries::biomes() const noexcept
	{
		return _biomes;
	}

	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept
	{
		return _prefabs;
	}

	const Registry<MapDefinition> &Registries::maps() const noexcept
	{
		return _maps;
	}
}
