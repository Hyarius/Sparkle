#include "world/biome_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/generator/placement_rules.hpp"
#include "world/prefab_definition.hpp"

#include <algorithm>
#include <cctype>
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
		paletteReader.forbidUnknown({"surface", "subsurface", "deep", "flora"});
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
					{"stairway", "gym", "city", "portCity", "normalPoi", "uncommonPoi", "rarePoi"});
				for (const auto &[slot, value] : prefabsReader.value().asObject())
				{
					if (value.isNull())
						continue;
					std::vector<std::string> pool =
						parsePrefabPool(value, p_reader.file(), prefabsReader.pathFor(slot), p_prefabs);
					if (pool.empty())
						continue;
					if (slot == "stairway")
					{
						for (const std::string &prefabId : pool)
						{
							const spk::Vector3Int &stairSize = p_prefabs.get(prefabId).size();
							if (stairSize.x != stairSize.y || stairSize.y != stairSize.z)
								throw JsonError(
									p_reader.file(),
									prefabsReader.pathFor(slot),
									"stairway prefab '" + prefabId + "' must be a cube");
						}
					}
					traits.prefabs.emplace(slot, std::move(pool));
				}
			}
			result.worldgen = std::move(traits);
		}
		return result;
	}
}
