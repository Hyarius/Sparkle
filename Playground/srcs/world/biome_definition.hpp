#pragma once

#include "core/json.hpp"
#include "core/weighted_pool.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/math/spk_vector3.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class VoxelRegistry;
	struct PrefabDefinition;
	template <typename TDefinition>
	class Registry;

	struct BiomePalette
	{
		using VoxelPool = WeightedPool<std::string>;

		// Terrain and climb entries are weighted voxel pools. JSON may still use the
		// old shorthand string for a single block, or an array of strings for equal odds.
		VoxelPool surface;
		VoxelPool subsurface;
		VoxelPool deep;
		// Blocks paved onto this biome's roads. Empty for biomes that pave no roads
		// (interiors).
		VoxelPool road;
		VoxelPool flora;
		// Climb step voxels the generator builds staircases/slopes from. Each is a pool:
		// a flight picks one voxel per block for visual variety. "stair" drives road
		// climbs (paired with a road block), "slope" wild climbs (paired with surface).
		// Empty pools fall back to the shared road-block staircase.
		VoxelPool stair;
		VoxelPool slope;
	};

	struct BiomeScenery
	{
		std::string prefabId;
		double density = 0.0; // expected instances per suitable world-plan cell
		int spacing = 1;	  // minimum center distance in voxel columns
		spk::Vector3Int prefabSize{};
	};

	struct BiomeWildStairs
	{
		bool configured = false;			  // missing block falls back to WorldGenConfig wild stair settings
		std::optional<bool> allowCrossZone;	  // permits wild climbs between adjacent zones
		std::optional<int> maxPerZone;		  // empty means no biome cap
		std::optional<int> maxLevels;		  // falls back to WorldGenConfig::maxWildStairLevels
		std::optional<double> spacingCells;	  // falls back to WorldGenConfig::wildStairSpacingCells
		std::optional<double> candidateRatio; // 0..1 chance to keep each suitable cliff candidate
	};

	// Optional per-biome knobs for the macro world generator. Biomes without this block
	// (interiors such as caves) are never picked when zones are assigned.
	struct BiomeWorldgenTraits
	{
		double heightShift = 0.0;			// strata-level bias for zones of this biome
		bool peak = false;					// biome hosts summits (mountain/volcano/tundra style)
		std::optional<spk::Color> mapColor; // zone fill on the preview map (absent = auto)
		// Per-biome entity prefab pools by slot ("gym", "city", "portCity",
		// "normalPoi", "uncommonPoi", "rarePoi"). Each slot holds one prefab id or a
		// list; the generator picks one entry at random per placement. Missing slots
		// fall back to the global placements.json rules.
		std::map<std::string, std::vector<std::string>> prefabs;
		// Each scenery entry has its own expected count and voxel-level spacing.
		std::vector<BiomeScenery> scenery;
		BiomeWildStairs wildStairs;
		// Climb prefab ids synthesized at load from the palette's stair/slope voxels
		// (see synthesizeClimbPrefabs). The flight pools carry several pre-mixed variants
		// per biome; the generator picks one per staircase segment. Empty when the biome
		// declares no stair/slope voxels, so the generator uses the shared fallback.
		std::vector<std::string> roadStairLengths;
		std::string roadStairPlatform;
		std::vector<std::string> wildSlopeLengths;
		std::string wildSlopePlatform;
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
