#include "core/registries.hpp"

#include "core/json.hpp"

#include <iostream>
#include <algorithm>

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
		Registry<Status> loadedStatuses;
		loadedStatuses.load(p_dataDirectory / "statuses", parseStatus);
		for (const std::string &statusId : loadedStatuses.ids())
		{
			resolveStatusReferences(
				loadedStatuses.getMutable(statusId), loadedStatuses, p_dataDirectory / "statuses" / (statusId + ".json"));
		}
		Registry<Ability> loadedAbilities;
		loadedAbilities.load(p_dataDirectory / "abilities", [&loadedStatuses](JsonReader &abilityReader) {
			return parseAbility(abilityReader, loadedStatuses);
		});
		Registry<ModelDefinition> loadedModels;
		loadedModels.load(p_dataDirectory / "models", parseModelDefinition);
		FeatRegistry loadedFeatBoards;
		loadedFeatBoards.load(p_dataDirectory / "featboards", loadedAbilities, loadedStatuses);
		Registry<AIBehaviour> loadedAI;
		loadedAI.load(p_dataDirectory / "ai", [&loadedAbilities, &loadedStatuses](JsonReader &p_reader) {
			return parseAIBehaviour(p_reader, loadedAbilities, loadedStatuses);
		});
		Registry<CreatureSpecies> loadedCreatures;
		loadedCreatures.load(p_dataDirectory / "creatures", [&loadedAbilities, &loadedFeatBoards](JsonReader &p_reader) {
			return parseCreatureSpecies(p_reader, loadedAbilities, loadedFeatBoards);
		});
		for (const std::string &speciesId : loadedCreatures.ids())
		{
			const CreatureSpecies &species = loadedCreatures.get(speciesId);
			for (const auto &[formId, form] : species.forms)
			{
				if (form.modelId != "placeholder-cube" && loadedModels.tryGet(form.modelId) == nullptr)
				{
					throw JsonError(
						p_dataDirectory / "creatures" / (speciesId + ".json"),
						"$.forms." + formId + ".model",
						"unknown model id '" + form.modelId + "'");
				}
			}
		}
		Registry<EncounterTable> loadedEncounterTables;
		loadedEncounterTables.load(p_dataDirectory / "encounter-tables", [&loadedAI](JsonReader &p_reader) {
			EncounterTable table = parseEncounterTable(p_reader);
			for (const EncounterTier &tier : table.tiers)
			{
				for (const WeightedEncounterTeam &team : tier.weightedTeams)
				{
					for (const EncounterTeamMember &member : team.team)
					{
						if (loadedAI.tryGet(member.aiId) == nullptr)
						{
							throw JsonError(
								p_reader.file(), "$.tiers", "unknown AI behaviour id '" + member.aiId + "'");
						}
					}
				}
			}
			return table;
		});
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
			for (std::size_t index = 0; index < map.trainers.size(); ++index)
			{
				if (loadedEncounterTables.tryGet(map.trainers[index].encounterTable) == nullptr)
				{
					throw JsonError(
						p_dataDirectory / "maps" / (mapId + ".json"),
						"$.trainers[" + std::to_string(index) + "].encounterTable",
						"unknown encounter table id '" + map.trainers[index].encounterTable + "'");
				}
			}
			for (std::size_t index = 0; index < map.portals.size(); ++index)
			{
				const MapPortal &portal = map.portals[index];
				const MapDefinition *targetMap = loadedMaps.tryGet(portal.target.map);
				if (targetMap == nullptr)
				{
					throw JsonError(
						p_dataDirectory / "maps" / (mapId + ".json"),
						"$.portals[" + std::to_string(index) + "].target.map",
						"unknown map id '" + portal.target.map + "'");
				}
				if (std::ranges::find(targetMap->portals, portal.target.portal, &MapPortal::name) == targetMap->portals.end())
				{
					throw JsonError(
						p_dataDirectory / "maps" / (mapId + ".json"),
						"$.portals[" + std::to_string(index) + "].target.portal",
						"unknown portal '" + portal.target.portal + "' in map '" + portal.target.map + "'");
				}
			}
		}

		_gameRules = std::move(loadedGameRules);
		_voxels = std::move(loadedVoxels);
		_statuses = std::move(loadedStatuses);
		_abilities = std::move(loadedAbilities);
		_models = std::move(loadedModels);
		_featBoards = std::move(loadedFeatBoards);
		_ai = std::move(loadedAI);
		_creatures = std::move(loadedCreatures);
		_encounterTables = std::move(loadedEncounterTables);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_maps = std::move(loadedMaps);
		std::cout << "Loaded " << _voxels.size() << " voxel definitions" << std::endl;
		std::cout << "Loaded " << _abilities.size() << " ability definitions" << std::endl;
		std::cout << "Loaded " << _statuses.size() << " status definitions" << std::endl;
		std::cout << "Loaded " << _creatures.size() << " creature species and "
				  << _models.size() << " model definitions" << std::endl;
		std::cout << "Loaded " << _featBoards.size() << " feat boards with "
				  << _featBoards.nodeCount() << " nodes" << std::endl;
		std::cout << "Loaded " << _ai.size() << " AI behaviours" << std::endl;
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

	const Registry<Status> &Registries::statuses() const noexcept
	{
		return _statuses;
	}

	const Registry<ModelDefinition> &Registries::models() const noexcept
	{
		return _models;
	}

	const FeatRegistry &Registries::featBoards() const noexcept
	{
		return _featBoards;
	}

	const Registry<AIBehaviour> &Registries::ai() const noexcept
	{
		return _ai;
	}

	const Registry<CreatureSpecies> &Registries::creatures() const noexcept
	{
		return _creatures;
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
