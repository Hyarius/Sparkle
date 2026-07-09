#include "world/biome_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/prefab_definition.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <utility>

namespace
{
	std::string requireNonEmptyString(pg::JsonReader &p_reader, const std::string &p_key)
	{
		std::string result = p_reader.require<std::string>(p_key);
		if (result.empty())
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_key), "value must not be empty");
		return result;
	}

	void requireVoxel(
		const pg::VoxelRegistry &p_voxels,
		const std::string &p_id,
		const pg::JsonReader &p_reader,
		const std::string &p_path)
	{
		if (p_id.empty() || p_voxels.tryGet(p_id) == nullptr)
			throw pg::JsonError(p_reader.file(), p_path, "unknown voxel id '" + p_id + "'");
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
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported biome version");

		BiomeDefinition result;
		result.displayName = requireNonEmptyString(p_reader, "displayName");
		JsonReader paletteReader = p_reader.child("palette");
		paletteReader.forbidUnknown({"surface", "subsurface", "deep", "road", "flora", "stair", "slope"});
		// "road"/"stair"/"slope" are voxel pools, each a single id or an array; realization
		// and the climb synthesizer pick from them (see synthesizeClimbPrefabs). requireVoxel
		// validates each entry. "road" is optional: interior biomes (caves) pave no roads.
		const auto parseVoxelPool = [&](const std::string &p_key) {
			std::vector<std::string> pool;
			if (!paletteReader.contains(p_key))
				return pool;
			const spk::JSON::Value &value = *paletteReader.value().find(p_key);
			const auto appendOne = [&](const spk::JSON::Value &p_entry, const std::string &p_path) {
				if (!p_entry.isString())
					throw JsonError(paletteReader.file(), p_path, "expected a voxel id string");
				std::string voxelId = p_entry.as<std::string>();
				requireVoxel(p_voxels, voxelId, paletteReader, p_path);
				pool.push_back(std::move(voxelId));
			};
			if (value.isArray())
			{
				const spk::JSON::Value::Array &entries = value.asArray();
				for (std::size_t index = 0; index < entries.size(); ++index)
					appendOne(entries[index], paletteReader.pathFor(p_key) + "[" + std::to_string(index) + "]");
			}
			else if (!value.isNull())
			{
				appendOne(value, paletteReader.pathFor(p_key));
			}
			return pool;
		};

		result.palette = {
			.surface = requireNonEmptyString(paletteReader, "surface"),
			.subsurface = requireNonEmptyString(paletteReader, "subsurface"),
			.deep = requireNonEmptyString(paletteReader, "deep"),
			.flora = paletteReader.require<std::vector<std::string>>("flora")};
		requireVoxel(p_voxels, result.palette.surface, paletteReader, paletteReader.pathFor("surface"));
		requireVoxel(p_voxels, result.palette.subsurface, paletteReader, paletteReader.pathFor("subsurface"));
		requireVoxel(p_voxels, result.palette.deep, paletteReader, paletteReader.pathFor("deep"));
		for (const std::string &flora : result.palette.flora)
			requireVoxel(p_voxels, flora, paletteReader, paletteReader.pathFor("flora"));
		result.palette.road = parseVoxelPool("road");
		result.palette.stair = parseVoxelPool("stair");
		result.palette.slope = parseVoxelPool("slope");

		if (p_reader.contains("worldgen"))
		{
			JsonReader worldgenReader = p_reader.child("worldgen");
			worldgenReader.forbidUnknown({"heightShift", "peak", "mapColor", "prefabs"});
			BiomeWorldgenTraits traits;
			if (worldgenReader.contains("heightShift"))
				traits.heightShift = worldgenReader.require<double>("heightShift");
			if (worldgenReader.contains("peak"))
				traits.peak = worldgenReader.require<bool>("peak");
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
						continue;
					std::vector<std::string> pool =
						parsePrefabPool(value, p_reader.file(), prefabsReader.pathFor(slot), p_prefabs);
					if (pool.empty())
						continue;
					traits.prefabs.emplace(slot, std::move(pool));
				}
			}
			result.worldgen = std::move(traits);
		}
		return result;
	}
}
