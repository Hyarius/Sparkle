#include "world/biome_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/prefab_definition.hpp"

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
		const std::string &p_id,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		if (p_id.empty() || p_voxels.tryGet(p_id) == nullptr)
		{
			throw pg::JsonError(p_reader.file(), p_path, "unknown voxel id '" + p_id + "'");
		}
	}

	double parseWeight(const spk::JSON::Value &p_value, const pg::JsonReader &p_reader, const std::string &p_path)
	{
		if (!p_value.isNumber())
		{
			throw pg::JsonError(p_reader.file(), p_path, "expected a numeric weight");
		}
		const double weight = p_value.as<double>();
		if (!std::isfinite(weight) || weight <= 0.0)
		{
			throw pg::JsonError(p_reader.file(), p_path, "weight must be greater than 0");
		}
		return weight;
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
			const auto appendOne = [&](const spk::JSON::Value &p_entry, const std::string &p_path) {
				if (!p_entry.isString())
				{
					throw JsonError(paletteReader.file(), p_path, "expected a voxel id string or weighted voxel object");
				}
				std::string voxelId = p_entry.as<std::string>();
				requireVoxel(p_voxels, voxelId, paletteReader, p_path);
				pool.push_back({.id = std::move(voxelId), .weight = 1.0});
			};
			const auto appendWeighted = [&](const spk::JSON::Value &p_entry, const std::string &p_path) {
				if (!p_entry.isObject())
				{
					appendOne(p_entry, p_path);
					return;
				}

				const spk::JSON::Value *voxelValue = p_entry.find("voxel");
				const spk::JSON::Value *idValue = p_entry.find("id");
				if (voxelValue == nullptr && idValue == nullptr)
				{
					for (const auto &[voxelId, weightValue] : p_entry.asObject())
					{
						requireVoxel(p_voxels, voxelId, paletteReader, p_path + "." + voxelId);
						pool.push_back(
							{.id = voxelId, .weight = parseWeight(weightValue, paletteReader, p_path + "." + voxelId)});
					}
					return;
				}
				if (voxelValue != nullptr && idValue != nullptr)
				{
					throw JsonError(paletteReader.file(), p_path, "use either 'voxel' or 'id', not both");
				}
				for (const auto &[key, unused] : p_entry.asObject())
				{
					(void)unused;
					if (key != "voxel" && key != "id" && key != "weight" && key != "chance")
					{
						throw JsonError(paletteReader.file(), p_path + "." + key, "unknown field");
					}
				}

				const spk::JSON::Value &selectedVoxel = voxelValue != nullptr ? *voxelValue : *idValue;
				if (!selectedVoxel.isString())
				{
					throw JsonError(paletteReader.file(), p_path, "expected a voxel id string");
				}
				std::string voxelId = selectedVoxel.as<std::string>();
				requireVoxel(p_voxels, voxelId, paletteReader, p_path);

				const spk::JSON::Value *weightValue = p_entry.find("weight");
				const spk::JSON::Value *chanceValue = p_entry.find("chance");
				if (weightValue != nullptr && chanceValue != nullptr)
				{
					throw JsonError(paletteReader.file(), p_path, "use either 'weight' or 'chance', not both");
				}
				const double weight = weightValue != nullptr   ? parseWeight(*weightValue, paletteReader, p_path + ".weight")
									  : chanceValue != nullptr ? parseWeight(*chanceValue, paletteReader, p_path + ".chance")
															   : 1.0;
				pool.push_back({.id = std::move(voxelId), .weight = weight});
			};
			if (value.isArray())
			{
				const spk::JSON::Value::Array &entries = value.asArray();
				for (std::size_t index = 0; index < entries.size(); ++index)
				{
					appendWeighted(entries[index], paletteReader.pathFor(p_key) + "[" + std::to_string(index) + "]");
				}
			}
			else if (value.isObject())
			{
				appendWeighted(value, paletteReader.pathFor(p_key));
			}
			else if (!value.isNull())
			{
				appendOne(value, paletteReader.pathFor(p_key));
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
			worldgenReader.forbidUnknown({"heightShift", "peak", "mapColor", "prefabs"});
			BiomeWorldgenTraits traits;
			if (worldgenReader.contains("heightShift"))
			{
				traits.heightShift = worldgenReader.require<double>("heightShift");
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
					{"scenery", "gym", "city", "portCity", "normalPoi", "uncommonPoi", "rarePoi"});
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
				for (const auto &[slot, value] : prefabsReader.value().asObject())
				{
					if (slot == "scenery" || value.isNull())
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
			result.worldgen = std::move(traits);
		}
		return result;
	}
}
