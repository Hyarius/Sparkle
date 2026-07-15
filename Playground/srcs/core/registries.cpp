#include "core/registries.hpp"

#include "core/combat_definition_validation.hpp"
#include "core/content_id.hpp"
#include "core/json.hpp"
#include "world/generator/climb_prefabs.hpp"
#include "world/generator/placement_rules.hpp"

#include <iostream>
#include <set>
#include <stdexcept>

namespace pg
{
	void Registries::loadAll(const std::filesystem::path &p_dataDirectory)
	{
		const std::filesystem::path gameRulesFile = p_dataDirectory / "config" / "game-rules.json";
		const spk::JSON::Value json = JsonLoader::parseFile(gameRulesFile);
		JsonReader reader(json, gameRulesFile);
		GameRules loadedGameRules = parseGameRules(reader);

		// Shapes load before voxels: every voxel references its geometry by shape id and
		// instantiates it through the catalog.
		ShapeCatalog loadedShapes;
		spk::loadJsonDirectory(loadedShapes, p_dataDirectory / "shapes", [](std::string_view p_id, JsonReader &p_reader) {
			ShapeDefinition definition = parseShapeDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
		VoxelFamilyCatalog loadedVoxelFamilies;
		spk::loadJsonDirectory(
			loadedVoxelFamilies,
			p_dataDirectory / "voxel_families",
			[&loadedShapes](std::string_view p_id, JsonReader &p_reader) {
				VoxelFamilyDefinition definition = parseVoxelFamilyDefinition(p_reader, loadedShapes);
				definition.id = p_id;
				return definition;
			});
		VoxelRegistry loadedVoxels;
		loadedVoxels.load(loadedShapes, loadedVoxelFamilies, p_dataDirectory / "voxels");
		// Prefabs load before biomes: a biome's worldgen block may reference prefabs.
		Registry<PrefabDefinition> loadedPrefabs;
		spk::loadJsonDirectory(loadedPrefabs, p_dataDirectory / "prefabs", [&loadedVoxels](std::string_view p_id, JsonReader &p_reader) {
			PrefabDefinition definition = parsePrefabDefinition(p_reader, loadedVoxels);
			definition.id = p_id;
			return definition;
		});
		Registry<BiomeDefinition> loadedBiomes;
		spk::loadJsonDirectory(loadedBiomes, p_dataDirectory / "biomes", [&loadedVoxels, &loadedPrefabs](std::string_view p_id, JsonReader &p_reader) {
			BiomeDefinition definition = parseBiomeDefinition(p_reader, loadedVoxels, loadedPrefabs);
			definition.id = p_id;
			return definition;
		});
		std::set<std::string> townPrefabIds;
		for (const std::string &biomeId : loadedBiomes.ids())
		{
			const BiomeDefinition &biome = loadedBiomes.get(biomeId);
			if (!biome.worldgen || !biome.worldgen->town)
			{
				continue;
			}
			const BiomeTown &town = *biome.worldgen->town;
			townPrefabIds.insert(town.creatureCenter);
			townPrefabIds.insert(town.shop);
			townPrefabIds.insert(town.gym);
			townPrefabIds.insert(town.port);
			townPrefabIds.insert(town.homes.begin(), town.homes.end());
		}
		for (const std::string &prefabId : townPrefabIds)
		{
			if (!loadedPrefabs.contains(prefabId))
			{
				throw std::runtime_error("town content references unknown prefab '" + prefabId + "'");
			}
			const PrefabDefinition &prefab = loadedPrefabs.get(prefabId);
			if (!prefab.entrance)
			{
				throw std::runtime_error("town prefab '" + prefabId + "' needs an explicit entrance contract");
			}
			if (prefab.tryAnchor(prefab.entrance->anchorName) == nullptr)
			{
				throw std::runtime_error("town prefab '" + prefabId + "' entrance references a missing anchor");
			}
		}
		// Interiors reference room prefabs, so they load after prefabs; each prefab's
		// own "interior" link is validated once both registries exist.
		Registry<InteriorDefinition> loadedInteriors;
		spk::loadJsonDirectory(loadedInteriors, p_dataDirectory / "interiors", [&loadedPrefabs](std::string_view p_id, JsonReader &p_reader) {
			InteriorDefinition definition = parseInteriorDefinition(p_reader, loadedPrefabs);
			definition.id = p_id;
			return definition;
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
		TownCompositionCatalog loadedTownCompositions;
		spk::loadJsonDirectory(loadedTownCompositions, p_dataDirectory / "worldgen" / "town_compositions", [&loadedPrefabs](std::string_view p_id, JsonReader &p_reader) {
			TownComposition composition = parseTownComposition(p_reader);
			composition.id = p_id;
			for (const TownSceneryRequest &request : composition.roadScenery)
			{
				if (!loadedPrefabs.contains(request.prefabId))
				{
					throw JsonError(p_reader.file(), p_reader.pathFor("roadScenery"), "unknown scenery prefab id '" + request.prefabId + "'");
				}
			}
			for (const TownSceneryRequest &request : composition.groundScenery)
			{
				if (!loadedPrefabs.contains(request.prefabId))
				{
					throw JsonError(p_reader.file(), p_reader.pathFor("groundScenery"), "unknown scenery prefab id '" + request.prefabId + "'");
				}
			}
			return composition;
		});

		// The combat domains parse in a fixed order for a stable load report, but never depend
		// on it: each keeps its status/object references as string ids, and the whole graph is
		// validated once below, after all three exist. That is what lets a status place an
		// object whose trigger removes that same status.
		Registry<StatusDefinition> loadedStatuses;
		spk::loadJsonDirectory(loadedStatuses, p_dataDirectory / "statuses", [](std::string_view p_id, JsonReader &p_reader) {
			requireContentId(p_id, p_reader.file(), p_reader.path(), "status id");
			StatusDefinition definition = parseStatusDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
		Registry<BattleObjectDefinition> loadedBattleObjects;
		spk::loadJsonDirectory(
			loadedBattleObjects,
			p_dataDirectory / "battle-objects",
			[](std::string_view p_id, JsonReader &p_reader) {
				requireContentId(p_id, p_reader.file(), p_reader.path(), "battle object id");
				BattleObjectDefinition definition = parseBattleObjectDefinition(p_reader);
				definition.id = p_id;
				return definition;
			});
		Registry<AbilityDefinition> loadedAbilities;
		spk::loadJsonDirectory(loadedAbilities, p_dataDirectory / "abilities", [](std::string_view p_id, JsonReader &p_reader) {
			requireContentId(p_id, p_reader.file(), p_reader.path(), "ability id");
			AbilityDefinition definition = parseAbilityDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
		validateCombatDefinitionGraph(loadedStatuses, loadedAbilities, loadedBattleObjects);

		// Feat Boards close the same transaction rather than opening a second one: their
		// conditions and rewards reference the combat definitions above, and an unresolved
		// reference must leave the combat registries as unpublished as the boards.
		const ConditionLimits limits = conditionLimits(loadedGameRules);
		Registry<FeatBoardDefinition> loadedFeatBoards;
		spk::loadJsonDirectory(loadedFeatBoards, p_dataDirectory / "featboards", [&limits](std::string_view p_id, JsonReader &p_reader) {
			requireContentId(p_id, p_reader.file(), p_reader.path(), "feat board id");
			FeatBoardDefinition definition = parseFeatBoardDefinition(p_reader, limits);
			definition.id = p_id;
			return definition;
		});
		validateFeatBoardGraph(loadedStatuses, loadedAbilities, loadedFeatBoards);

		// The rest of the definition graph. It all still parses into locals, for the same reason:
		// an encounter that names a species that does not exist has to leave the species registry
		// as unpublished as the encounter table.
		Registry<AIBehaviourDefinition> loadedAiBehaviours;
		spk::loadJsonDirectory(loadedAiBehaviours, p_dataDirectory / "ai", [](std::string_view p_id, JsonReader &p_reader) {
			requireContentId(p_id, p_reader.file(), p_reader.path(), "AI behaviour id");
			AIBehaviourDefinition definition = parseAIBehaviourDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
		validateAIGraph(loadedStatuses, loadedAbilities, loadedAiBehaviours);

		Registry<CreatureSpeciesDefinition> loadedSpecies;
		spk::loadJsonDirectory(
			loadedSpecies,
			p_dataDirectory / "creatures",
			[&loadedGameRules, &limits](std::string_view p_id, JsonReader &p_reader) {
				requireContentId(p_id, p_reader.file(), p_reader.path(), "species id");
				CreatureSpeciesDefinition definition = parseCreatureSpeciesDefinition(p_reader, loadedGameRules, limits);
				definition.id = p_id;
				return definition;
			});

		// Everything a species could not prove alone - and the one context every later derivation
		// reads, whether from these locals or from the published registries.
		const DerivationContext context{
			.gameRules = &loadedGameRules,
			.statuses = &loadedStatuses,
			.abilities = &loadedAbilities,
			.featBoards = &loadedFeatBoards,
			.species = &loadedSpecies};
		validateSpeciesGraph(context);

		// Battle boards need the prefabs above: their geometry is an ordinary prefab, validated at
		// the identity transform step 05 will stamp it with.
		Registry<HandcraftedBattleBoardDefinition> loadedBattleBoards;
		spk::loadJsonDirectory(
			loadedBattleBoards,
			p_dataDirectory / "battle-boards",
			[&loadedPrefabs](std::string_view p_id, JsonReader &p_reader) {
				requireContentId(p_id, p_reader.file(), p_reader.path(), "battle board id");
				HandcraftedBattleBoardDefinition definition = parseHandcraftedBattleBoardDefinition(p_reader, loadedPrefabs);
				definition.id = p_id;
				return definition;
			});

		Registry<EncounterDefinition> loadedEncounters;
		spk::loadJsonDirectory(
			loadedEncounters,
			p_dataDirectory / "encounter-tables",
			[&loadedBattleBoards](std::string_view p_id, JsonReader &p_reader) {
				requireContentId(p_id, p_reader.file(), p_reader.path(), "encounter id");
				EncounterDefinition definition = parseEncounterDefinition(p_reader, loadedBattleBoards);
				definition.id = p_id;
				return definition;
			});
		validateEncounterGraph(loadedEncounters, loadedAiBehaviours, context);
		validateBiomeEncounterLinks(loadedBiomes, loadedEncounters);

		// The starter team is content too, so it is validated here rather than trusted at boot: it
		// is built once against the locals, and the value main() constructs is derived from the
		// published registries by the exact same code.
		const std::filesystem::path newGameFile = p_dataDirectory / "config" / "new-game.json";
		const spk::JSON::Value newGameJson = JsonLoader::parseFile(newGameFile);
		JsonReader newGameReader(newGameJson, newGameFile);
		NewGameDefinition loadedNewGame = parseNewGameDefinition(newGameReader);
		(void)makeNewPlayerData(loadedNewGame, context);

		_gameRules = std::move(loadedGameRules);
		_shapes = std::move(loadedShapes);
		_voxelFamilies = std::move(loadedVoxelFamilies);
		_voxels = std::move(loadedVoxels);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_interiors = std::move(loadedInteriors);
		_placementRules = std::move(loadedPlacementRules);
		_townCompositions = std::move(loadedTownCompositions);
		_statuses = std::move(loadedStatuses);
		_abilities = std::move(loadedAbilities);
		_battleObjects = std::move(loadedBattleObjects);
		_featBoards = std::move(loadedFeatBoards);
		_aiBehaviours = std::move(loadedAiBehaviours);
		_species = std::move(loadedSpecies);
		_battleBoards = std::move(loadedBattleBoards);
		_encounters = std::move(loadedEncounters);
		_newGame = std::move(loadedNewGame);
		std::cout << "Loaded " << _voxels.typeCount() << " voxel types containing "
				  << _voxels.runtimeStateCount() << " runtime states, " << _biomes.size()
				  << " biome definitions, " << _prefabs.size() << " prefabs, and " << _interiors.size()
				  << " interiors" << std::endl;
		std::cout << "Loaded " << _statuses.size() << " statuses, " << _abilities.size() << " abilities, and "
				  << _battleObjects.size() << " battle objects" << std::endl;

		std::size_t featNodeCount = 0;
		for (const auto &[boardId, board] : _featBoards)
		{
			(void)boardId;
			featNodeCount += board.nodes.size();
		}
		std::cout << "Loaded " << _featBoards.size() << " feat boards containing " << featNodeCount << " nodes"
				  << std::endl;

		std::size_t encounterTeamCount = 0;
		for (const auto &[encounterId, encounter] : _encounters)
		{
			(void)encounterId;
			for (const EncounterTierDefinition &tier : encounter.tiers)
			{
				encounterTeamCount += tier.teams.size();
			}
		}
		std::cout << "Loaded " << _species.size() << " creature species, " << _aiBehaviours.size()
				  << " AI behaviours, " << _battleBoards.size() << " handcrafted battle boards, and "
				  << _encounters.size() << " encounter tables holding " << encounterTeamCount << " teams" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept
	{
		return _gameRules;
	}
	const ShapeCatalog &Registries::shapes() const noexcept
	{
		return _shapes;
	}
	const VoxelFamilyCatalog &Registries::voxelFamilies() const noexcept
	{
		return _voxelFamilies;
	}
	const VoxelRegistry &Registries::voxels() const noexcept
	{
		return _voxels;
	}
	const Registry<BiomeDefinition> &Registries::biomes() const noexcept
	{
		return _biomes;
	}
	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept
	{
		return _prefabs;
	}
	const Registry<InteriorDefinition> &Registries::interiors() const noexcept
	{
		return _interiors;
	}
	const PlanPlacementRules &Registries::placementRules() const noexcept
	{
		return _placementRules;
	}
	const TownCompositionCatalog &Registries::townCompositions() const noexcept
	{
		return _townCompositions;
	}
	const Registry<StatusDefinition> &Registries::statuses() const noexcept
	{
		return _statuses;
	}
	const Registry<AbilityDefinition> &Registries::abilities() const noexcept
	{
		return _abilities;
	}
	const Registry<BattleObjectDefinition> &Registries::battleObjects() const noexcept
	{
		return _battleObjects;
	}
	const Registry<FeatBoardDefinition> &Registries::featBoards() const noexcept
	{
		return _featBoards;
	}
	const Registry<AIBehaviourDefinition> &Registries::aiBehaviours() const noexcept
	{
		return _aiBehaviours;
	}
	const Registry<CreatureSpeciesDefinition> &Registries::species() const noexcept
	{
		return _species;
	}
	const Registry<HandcraftedBattleBoardDefinition> &Registries::battleBoards() const noexcept
	{
		return _battleBoards;
	}
	const Registry<EncounterDefinition> &Registries::encounters() const noexcept
	{
		return _encounters;
	}
	const NewGameDefinition &Registries::newGame() const noexcept
	{
		return _newGame;
	}
}
