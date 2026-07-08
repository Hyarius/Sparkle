#pragma once

#include "world/chunk_provider.hpp"
#include "world/generator/world_plan.hpp"

#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pg
{
	class Registries;
	struct PrefabDefinition;

	// Realizes a WorldPlan as voxels, one chunk at a time:
	//  - every plan cell becomes a flat blocksPerCell x blocksPerCell plateau whose
	//    surface sits at groundLevelTop + level * blocksPerLevel (the 3-block strata),
	//    topped with the biome surface block over subsurface/deep layers;
	//  - rivers carve a channel along the cell's water connections, lakes flood the
	//    whole cell, the ocean surrounds the landmass;
	//  - road cells get a 3-wide paved strip toward each connected road neighbor,
	//    lifted onto solid piers where the road crosses water (bridges);
	//  - the plan's prefab placements (stairways between strata, buildings, flora)
	//    are stamped last, rotated to their orientation, with an optional solid
	//    foundation column grown down to the terrain.
	// The plan is immutable and lookups are pure, so fill() is safe on worker threads.
	class PlanChunkProvider final : public IChunkProvider
	{
	private:
		struct ResolvedPlacement
		{
			const PrefabDefinition *prefab = nullptr;
			spk::Vector3Int worldMin{};
			spk::Vector3Int rotatedSize{};
			spk::VoxelOrientation orientation = spk::VoxelOrientation::PositiveZ;
			bool foundation = false;
		};

		struct BiomeBlocks
		{
			std::int32_t surface = -1;
			std::int32_t subsurface = -1;
			std::int32_t deep = -1;
			std::int32_t road = -1;
		};

		struct Column
		{
			int groundTop = -1;      // highest solid block (-1: no ground in this column)
			std::int32_t surfaceId = -1;
			std::int32_t subsurfaceId = -1;
			std::int32_t deepId = -1;
			int waterY = -1;         // water block at this height (-1: none)
			int subsurfaceDepth = 3; // blocks of subsurface under the surface block
		};

		const WorldPlan &_plan;
		std::vector<BiomeBlocks> _biomeBlocks; // indexed like _plan.biomes
		std::int32_t _road = -1;
		std::int32_t _water = -1;
		std::int32_t _sand = -1;
		std::int32_t _stone = -1;
		std::vector<ResolvedPlacement> _placements;

		[[nodiscard]] Column _column(int p_worldX, int p_worldZ) const;
		void _stamp(spk::VoxelChunk &p_chunk, const ResolvedPlacement &p_placement) const;

	public:
		PlanChunkProvider(const Registries &p_registries, const WorldPlan &p_plan);

		void fill(spk::VoxelChunk &p_chunk) const override;

		// Surface y of the column (for spawn placement); world coordinates.
		[[nodiscard]] int surfaceHeight(int p_worldX, int p_worldZ) const;
		[[nodiscard]] int maxHeight() const noexcept;
		// A road cell near the first gym: open ground, always connected to the network.
		[[nodiscard]] spk::Vector3Int spawnCell() const;
	};
}
