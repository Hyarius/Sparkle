#include "world/biome_definition.hpp"

#include "core/json.hpp"
#include "voxel/voxel_registry.hpp"

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
	BiomeDefinition parseBiomeDefinition(JsonReader &p_reader, const VoxelRegistry &p_voxels)
	{
		p_reader.forbidUnknown({"version", "displayName", "palette"});
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
		return result;
	}
}
