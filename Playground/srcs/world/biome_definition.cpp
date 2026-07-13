#include "world/biome_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/prefab_definition.hpp"
#include "world/weighted_id_parser.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <utility>

namespace
{
	std::string requireNonEmptyString(pg::JsonReader &p_reader, const std::string &p_key)
	{
		std::string result = p_reader.require<std::string>(p_key);
		if (result.empty())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must not be empty");
		}
		return result;
	}

	void requireVoxel(
		const pg::VoxelRegistry &p_voxels,
		const pg::VoxelReference &p_reference,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		const pg::VoxelDefinition *definition =
			p_reference.id.empty() ? nullptr : p_voxels.tryGet(p_reference.id);
		if (definition == nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "unknown voxel id '" + p_reference.id + "'");
		}
		if (definition->tryState(p_reference.state) == nullptr)
		{
			throw pg::JsonError(
				p_reader.file(),
				p_path,
				"voxel '" + p_reference.id + "' has no state " + std::to_string(p_reference.state.value));
		}
	}

	double requireFiniteDouble(pg::JsonReader &p_reader, const std::string &p_key)
	{
		const double result = p_reader.require<double>(p_key);
		if (!std::isfinite(result))
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must be finite");
		}
		return result;
	}
}

namespace pg
{
	BiomeDefinition parseBiomeDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels,
		const Registry<PrefabDefinition> &p_prefabs)
	{
		p_reader.forbidUnknown({"version", "displayName", "palette", "worldgen"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported biome version");
		}

		BiomeDefinition result;
		result.displayName = requireNonEmptyString(p_reader, "displayName");
		JsonReader paletteReader = p_reader.child("palette");
		paletteReader.forbidUnknown({"surface", "subsurface", "deep", "road", "flora", "stair", "slope"});
		// Palette entries are weighted voxel pools. Accepted shapes:
		//   "grass-block"
		//   ["grass-block", "flowered-grass"]
		//   [{"voxel": "grass-block", "weight": 80}, {"voxel": "flowered-grass", "weight": 20}]
		// A single object map is also accepted: {"grass-block": 80, "flowered-grass": 20}.
		const auto parseVoxelPool = [&](const std::string &p_key) {
			BiomePalette::VoxelPool pool;
			if (!paletteReader.contains(p_key))
			{
				return pool;
			}
			const spk::JSON::Value &value = *paletteReader.value().find(p_key);
			if (value.isNull())
			{
				return pool;
			}
			for (WeightedId &entry : parseWeightedIds(value, paletteReader, paletteReader.pathFor(p_key)))
			{
				if (!entry.id.has_value())
				{
					throw JsonError(paletteReader.file(), entry.path, "expected a voxel id string");
				}
				VoxelReference reference{
					.id = std::move(*entry.id),
					.state = entry.state.value_or(spk::VoxelStateId{})};
				requireVoxel(p_voxels, reference, paletteReader, entry.path);
				pool.add(std::move(reference), entry.weight);
			}
			return pool;
		};
		const auto parseRequiredVoxelPool = [&](const std::string &p_key) {
			BiomePalette::VoxelPool pool = parseVoxelPool(p_key);
			if (!paletteReader.contains(p_key))
			{
				throw JsonError(paletteReader.file(), paletteReader.pathFor(p_key), "missing required field");
			}
			if (pool.empty())
			{
				throw JsonError(paletteReader.file(), paletteReader.pathFor(p_key), "pool must contain at least one voxel");
			}
			return pool;
		};

		result.palette = {
			.surface = parseRequiredVoxelPool("surface"),
			.subsurface = parseRequiredVoxelPool("subsurface"),
			.deep = parseRequiredVoxelPool("deep"),
			.flora = parseRequiredVoxelPool("flora")};
		result.palette.road = parseVoxelPool("road");
		result.palette.stair = parseVoxelPool("stair");
		result.palette.slope = parseVoxelPool("slope");

		if (p_reader.contains("worldgen"))
		{
			JsonReader worldgenReader = p_reader.child("worldgen");
			worldgenReader.forbidUnknown({"heightShift", "peak", "mapColor", "prefabs", "wildStairs"});
			BiomeWorldgenTraits traits;
			if (worldgenReader.contains("heightShift"))
			{
				traits.heightShift = requireFiniteDouble(worldgenReader, "heightShift");
			}
			if (worldgenReader.contains("peak"))
			{
				traits.peak = worldgenReader.require<bool>("peak");
			}
			if (worldgenReader.contains("mapColor"))
			{
				try
				{
					traits.mapColor = spk::Color(worldgenReader.require<std::string>("mapColor"));
				} catch (const std::invalid_argument &exception)
				{
					throw JsonError(p_reader.file(), worldgenReader.pathFor("mapColor"), exception.what());
				}
			}
			if (worldgenReader.contains("prefabs"))
			{
				JsonReader prefabsReader = worldgenReader.child("prefabs");
				prefabsReader.forbidUnknown(
					{"scenery", "townScenery", "town", "gym", "city", "portCity", "normalPoi", "uncommonPoi", "rarePoi"});
				if (prefabsReader.contains("town"))
				{
					JsonReader townReader = prefabsReader.child("town");
					townReader.forbidUnknown({"creatureCenter", "shop", "gym", "port", "homes"});
					BiomeTown town;
					const auto requiredPrefab = [&](const std::string &p_key, bool p_required) {
						if (!townReader.contains(p_key))
						{
							if (p_required)
							{
								throw JsonError(townReader.file(), townReader.pathFor(p_key), "missing required town building");
							}
							return std::string{};
						}
						const std::string id = requireNonEmptyString(townReader, p_key);
						if (p_prefabs.tryGet(id) == nullptr)
						{
							throw JsonError(townReader.file(), townReader.pathFor(p_key), "unknown prefab id '" + id + "'");
						}
						const PrefabDefinition &prefab = p_prefabs.get(id);
						const PrefabAnchor *door = prefab.tryAnchor("door");
						if (door == nullptr || door->position.z != 0)
						{
							throw JsonError(townReader.file(), townReader.pathFor(p_key), "town prefab needs a local -Z 'door' anchor");
						}
						return id;
					};
					town.creatureCenter = requiredPrefab("creatureCenter", true);
					town.shop = requiredPrefab("shop", true);
					town.gym = requiredPrefab("gym", true);
					town.port = requiredPrefab("port", true);
					town.homes = parsePrefabPool(*townReader.value().find("homes"), townReader.file(), townReader.pathFor("homes"), p_prefabs);
					if (town.homes.empty())
					{
						throw JsonError(townReader.file(), townReader.pathFor("homes"), "town needs at least one home prefab");
					}
					for (const std::string &homeId : town.homes)
					{
						const PrefabAnchor *door = p_prefabs.get(homeId).tryAnchor("door");
						if (door == nullptr || door->position.z != 0)
							throw JsonError(townReader.file(), townReader.pathFor("homes"), "town home prefab needs a local -Z 'door' anchor");
					}
					traits.town = std::move(town);
				}
				if (prefabsReader.contains("scenery"))
				{
					const spk::JSON::Value *sceneryValue = prefabsReader.value().find("scenery");
					if (!sceneryValue->isNull())
					{
						std::set<std::string> prefabIds;
						for (JsonReader sceneryReader : prefabsReader.childArray("scenery"))
						{
							sceneryReader.forbidUnknown({"prefab", "density", "spacing"});
							BiomeScenery scenery{
								.prefabId = requireNonEmptyString(sceneryReader, "prefab"),
								.density = sceneryReader.require<double>("density")};
							const PrefabDefinition *prefab = p_prefabs.tryGet(scenery.prefabId);
							if (prefab == nullptr)
							{
								throw JsonError(
									p_reader.file(),
									sceneryReader.pathFor("prefab"),
									"unknown prefab id '" + scenery.prefabId + "'");
							}
							scenery.prefabSize = prefab->size();
							scenery.spacing = sceneryReader.optional<int>(
								"spacing", std::max(scenery.prefabSize.x, scenery.prefabSize.z));
							if (scenery.density < 0.0)
							{
								throw JsonError(
									p_reader.file(),
									sceneryReader.pathFor("density"),
									"density must be zero or greater");
							}
							if (scenery.spacing < 1)
							{
								throw JsonError(
									p_reader.file(),
									sceneryReader.pathFor("spacing"),
									"spacing must be at least 1");
							}
							if (!prefabIds.insert(scenery.prefabId).second)
							{
								throw JsonError(
									p_reader.file(),
									sceneryReader.pathFor("prefab"),
									"duplicate scenery prefab id '" + scenery.prefabId + "'");
							}
							traits.scenery.push_back(std::move(scenery));
						}
					}
				}
				if (prefabsReader.contains("townScenery"))
				{
					std::set<std::string> prefabIds;
					for (JsonReader sceneryReader : prefabsReader.childArray("townScenery"))
					{
						sceneryReader.forbidUnknown({"prefab", "density", "spacing", "placement"});
						BiomeScenery scenery{.prefabId = requireNonEmptyString(sceneryReader, "prefab"), .density = sceneryReader.require<double>("density")};
						const std::string placement = sceneryReader.optional<std::string>("placement", "anywhere");
						if (placement != "anywhere" && placement != "roadside")
						{
							throw JsonError(
								p_reader.file(), sceneryReader.pathFor("placement"),
								"town scenery placement must be 'anywhere' or 'roadside'");
						}
						scenery.roadside = placement == "roadside";
						const PrefabDefinition *prefab = p_prefabs.tryGet(scenery.prefabId);
						if (prefab == nullptr)
						{
							throw JsonError(p_reader.file(), sceneryReader.pathFor("prefab"), "unknown prefab id '" + scenery.prefabId + "'");
						}
						scenery.prefabSize = prefab->size();
						scenery.spacing = sceneryReader.optional<int>("spacing", std::max(scenery.prefabSize.x, scenery.prefabSize.z));
						if (scenery.density < 0.0 || scenery.spacing < 1)
						{
							throw JsonError(p_reader.file(), sceneryReader.pathFor("density"), "town scenery needs non-negative density and positive spacing");
						}
						if (!prefabIds.insert(scenery.prefabId).second)
						{
							throw JsonError(p_reader.file(), sceneryReader.pathFor("prefab"), "duplicate town scenery prefab id '" + scenery.prefabId + "'");
						}
						traits.townScenery.push_back(std::move(scenery));
					}
				}
				for (const auto &[slot, value] : prefabsReader.value().asObject())
				{
					if (slot == "scenery" || slot == "townScenery" || slot == "town" || value.isNull())
					{
						continue;
					}
					std::vector<std::string> pool =
						parsePrefabPool(value, p_reader.file(), prefabsReader.pathFor(slot), p_prefabs);
					if (pool.empty())
					{
						continue;
					}
					traits.prefabs.emplace(slot, std::move(pool));
				}
			}
			if (worldgenReader.contains("wildStairs"))
			{
				JsonReader wildReader = worldgenReader.child("wildStairs");
				wildReader.forbidUnknown({"allowCrossZone", "maxPerZone", "maxLevels", "spacingCells", "candidateRatio"});
				traits.wildStairs.configured = true;
				if (wildReader.contains("allowCrossZone"))
				{
					traits.wildStairs.allowCrossZone = wildReader.require<bool>("allowCrossZone");
				}
				if (wildReader.contains("maxPerZone"))
				{
					const spk::JSON::Value *maxValue = wildReader.value().find("maxPerZone");
					if (maxValue != nullptr && !maxValue->isNull())
					{
						const int maxPerZone = wildReader.require<int>("maxPerZone");
						if (maxPerZone < 0)
						{
							throw JsonError(
								p_reader.file(),
								wildReader.pathFor("maxPerZone"),
								"maxPerZone must be zero or greater");
						}
						traits.wildStairs.maxPerZone = maxPerZone;
					}
				}
				if (wildReader.contains("maxLevels"))
				{
					const int maxLevels = wildReader.require<int>("maxLevels");
					if (maxLevels < 1)
					{
						throw JsonError(
							p_reader.file(),
							wildReader.pathFor("maxLevels"),
							"maxLevels must be at least 1");
					}
					traits.wildStairs.maxLevels = maxLevels;
				}
				if (wildReader.contains("spacingCells"))
				{
					const double spacing = requireFiniteDouble(wildReader, "spacingCells");
					if (spacing < 0.0)
					{
						throw JsonError(
							p_reader.file(),
							wildReader.pathFor("spacingCells"),
							"spacingCells must be zero or greater");
					}
					traits.wildStairs.spacingCells = spacing;
				}
				if (wildReader.contains("candidateRatio"))
				{
					const double ratio = requireFiniteDouble(wildReader, "candidateRatio");
					if (ratio < 0.0 || ratio > 1.0)
					{
						throw JsonError(
							p_reader.file(),
							wildReader.pathFor("candidateRatio"),
							"candidateRatio must be between 0 and 1");
					}
					traits.wildStairs.candidateRatio = ratio;
				}
			}
			result.worldgen = std::move(traits);
		}
		return result;
	}
}
