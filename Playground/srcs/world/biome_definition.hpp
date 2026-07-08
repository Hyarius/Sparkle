#pragma once

#include "structures/graphics/geometry/spk_color.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;
	class VoxelRegistry;
	struct PrefabDefinition;
	template <typename TDefinition>
	class Registry;

	struct BiomePalette
	{
		std::string surface;
		std::string subsurface;
		std::string deep;
		std::vector<std::string> flora;
	};

	// Optional per-biome knobs for the macro world generator. Biomes without this block
	// (interiors such as caves) are never picked when zones are assigned.
	struct BiomeWorldgenTraits
	{
		double heightShift = 0.0;            // strata-level bias for zones of this biome
		bool peak = false;                   // biome hosts summits (mountain/volcano/tundra style)
		std::optional<spk::Color> mapColor;  // zone fill on the preview map (absent = auto)
		// Per-biome prefab pools by slot ("stairway", "gym", "city", "portCity",
		// "normalPoi", "uncommonPoi", "rarePoi"). Each slot holds one prefab id or a
		// list; the generator picks one entry at random per placement. Missing slots
		// fall back to the global worldgen/placements.json rules.
		std::map<std::string, std::vector<std::string>> prefabs;
	};

	struct BiomeDefinition
	{
		std::string id;
		std::string displayName;
		BiomePalette palette;
		std::optional<BiomeWorldgenTraits> worldgen;
	};

	[[nodiscard]] BiomeDefinition parseBiomeDefinition(
		JsonReader &p_reader,
		const VoxelRegistry &p_voxels,
		const Registry<PrefabDefinition> &p_prefabs);
}
