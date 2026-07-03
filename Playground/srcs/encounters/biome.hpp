#pragma once

#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class VoxelRegistry;
	template <typename TDefinition>
	class Registry;
	struct EncounterTable;

	struct BiomePalette
	{
		std::string surface;
		std::string subsurface;
		std::string deep;
		std::vector<std::string> flora;
	};

	struct BiomeEncounterRule
	{
		std::string trigger;
		const EncounterTable *table = nullptr;
		double chancePerStep = 0.0;
	};

	struct BiomeDefinition
	{
		std::string id;
		std::string displayName;
		BiomePalette palette;
		std::vector<BiomeEncounterRule> encounterRules;
	};

	[[nodiscard]] BiomeDefinition parseBiomeDefinition(
		JsonReader &p_reader,
		const Registry<EncounterTable> &p_encounterTables,
		const VoxelRegistry &p_voxels);
}
