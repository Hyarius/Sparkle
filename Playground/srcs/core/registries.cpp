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

		// Shapes load before voxels: every voxel references its geometry by shape id and
		// instantiates it through the catalog.
		ShapeCatalog loadedShapes;
		spk::loadJsonDirectory(loadedShapes, p_dataDirectory / "shapes", [](std::string_view p_id, JsonReader &p_reader) {
			ShapeDefinition definition = parseShapeDefinition(p_reader);
			definition.id = p_id;
			return definition;
		});
		VoxelRegistry loadedVoxels;
		loadedVoxels.load(loadedShapes, p_dataDirectory / "voxels");
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

		_gameRules = std::move(loadedGameRules);
		_shapes = std::move(loadedShapes);
		_voxels = std::move(loadedVoxels);
		_biomes = std::move(loadedBiomes);
		_prefabs = std::move(loadedPrefabs);
		_interiors = std::move(loadedInteriors);
		_placementRules = std::move(loadedPlacementRules);
		std::cout << "Loaded " << _voxels.typeCount() << " voxel types containing "
				  << _voxels.runtimeStateCount() << " runtime states, " << _biomes.size()
				  << " biome definitions, " << _prefabs.size() << " prefabs, and " << _interiors.size()
				  << " interiors" << std::endl;
	}

	const GameRules &Registries::gameRules() const noexcept { return _gameRules; }
	const ShapeCatalog &Registries::shapes() const noexcept { return _shapes; }
	const VoxelRegistry &Registries::voxels() const noexcept { return _voxels; }
	const Registry<BiomeDefinition> &Registries::biomes() const noexcept { return _biomes; }
	const Registry<PrefabDefinition> &Registries::prefabs() const noexcept { return _prefabs; }
	const Registry<InteriorDefinition> &Registries::interiors() const noexcept { return _interiors; }
	const PlanPlacementRules &Registries::placementRules() const noexcept { return _placementRules; }
}
