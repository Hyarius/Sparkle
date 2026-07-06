#pragma once

#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class VoxelRegistry;

	struct BiomePalette
	{
		std::string surface;
		std::string subsurface;
		std::string deep;
		std::vector<std::string> flora;
	};

	struct BiomeDefinition
	{
		std::string id;
		std::string displayName;
		BiomePalette palette;
	};

	[[nodiscard]] BiomeDefinition parseBiomeDefinition(JsonReader &p_reader, const VoxelRegistry &p_voxels);
}
