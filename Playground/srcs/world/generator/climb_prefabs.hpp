#pragma once

namespace pg
{
	class VoxelRegistry;
	struct BiomeDefinition;
	struct PrefabDefinition;
	template <typename TDefinition>
	class Registry;

	// Builds every biome's staircases and slopes from voxels instead of hand-authored
	// prefab files. For each worldgen biome that declares "stair"/"slope" voxels in its
	// palette this synthesizes flight prefabs (the fixed 3x3x3 ramp shape, its step blocks
	// sampled per cell from the pool so a flight reads as a mix rather than one repeated
	// block) plus the single road/surface platform pad, inserts them into the prefab
	// registry under generated ids ("<biome>-stair-length#<n>" / "<biome>-stair-platform"
	// on roads, "<biome>-slope-length#<n>" / "<biome>-slope-platform" in the wild), and
	// records those ids on the biome's worldgen traits so the generator can pick among them.
	// Biomes without stair/slope voxels are left to the shared road-block staircase.
	void synthesizeClimbPrefabs(
		Registry<PrefabDefinition> &p_prefabs,
		Registry<BiomeDefinition> &p_biomes,
		const VoxelRegistry &p_voxels);
}
