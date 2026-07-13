#pragma once

#include "core/weighted_pool.hpp"
#include "world/generator/world_plan.hpp"

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_ids.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace spk
{
	class VoxelChunk;
}

namespace pg
{
	class Registries;
	struct PrefabDefinition;

	// Realizes a WorldPlan as voxels, one chunk at a time:
	//  - every plan cell becomes a blocksPerCell x blocksPerCell height plate centered
	//    on groundLevelTop + level * blocksPerLevel (the 3-block strata), with subtle
	//    deterministic +/-1 voxel Perlin relief and material-matched slab transitions;
	//  - rivers carve a channel along the cell's water connections, lakes flood the
	//    whole cell, the ocean surrounds the landmass;
	//  - road cells get a 3-wide paved strip toward each connected road neighbor,
	//    lifted onto solid piers where the road crosses water (bridges);
	//  - the plan's prefab placements (stairways between strata, buildings, flora)
	//    are stamped last, rotated to their orientation, with an optional solid
	//    foundation column grown down to the terrain.
	// The plan is immutable and lookups are pure, so fill() is safe on worker threads.
	class PlanChunkProvider final
	{
	private:
		struct ResolvedPlacement
		{
			const PrefabDefinition *definition = nullptr;
			spk::Vector3Int worldMin{};	   // min corner of the rotated bounding box
			spk::Vector3Int rotatedSize{}; // extents of that box
			spk::Vector3Int destination{}; // world cell the prefab's pivot lands on
			int realizationMinY = 0;	   // lowest terrain/support or prefab voxel written
			spk::VoxelOrientation orientation = spk::VoxelOrientation::PositiveZ;
			bool foundation = false;
		};

		struct BiomeBlocks
		{
			using VoxelPool = WeightedPool<spk::VoxelRuntimeId>;

			VoxelPool surface;
			VoxelPool subsurface;
			VoxelPool deep;
			VoxelPool road; // paved per column from this pool
		};

		struct Column
		{
			int groundTop = -1; // highest solid block (-1: no ground in this column)
			spk::VoxelRuntimeId surfaceId{};
			spk::VoxelRuntimeId subsurfaceId{};
			spk::VoxelRuntimeId deepId{};
			spk::VoxelRuntimeId slabId{}; // optional half-height cap at groundTop + 1
			int waterY = -1;			  // water block at this height (-1: none)
			int subsurfaceDepth = 3;	  // blocks of subsurface under the surface block
		};

		const WorldPlan &_plan;
		std::vector<BiomeBlocks> _biomeBlocks; // indexed like _plan.biomes
		BiomeBlocks _fallbackBlocks;		   // for land cells outside any zone
		spk::VoxelRuntimeId _road{};
		spk::VoxelRuntimeId _water{};
		spk::VoxelRuntimeId _sand{};
		spk::VoxelRuntimeId _stone{};
		std::map<spk::VoxelRuntimeId, spk::VoxelRuntimeId> _slabByBase;
		std::vector<ResolvedPlacement> _placements;
		// Town buildings have a level door selected by the planner. Their complete
		// content footprints are cut or filled to that level before prefab stamping,
		// making foundation placement safe across macro-cell height boundaries.
		std::map<std::pair<int, int>, int> _townBuildingGroundTop;
		// Exact urban paving is indexed once when the immutable provider is built;
		// chunk columns never scan every town record.
		std::map<std::pair<int, int>, int> _urbanRoadSurfaceY;
		// Authored prefab/town/stair columns and a one-column guard ring stay at their
		// planned elevation. This keeps doors, foundations and macro stair exits exact.
		std::set<std::pair<int, int>> _terrainVariationBlocked;
		std::vector<std::uint64_t> _heightNoiseSeeds;
		std::uint64_t _terrainTransitionNoiseSeed = 0;

		[[nodiscard]] int _terrainOffset(int p_worldX, int p_worldZ) const;
		[[nodiscard]] Column _column(int p_worldX, int p_worldZ) const;
		void _stamp(spk::VoxelChunk &p_chunk, const ResolvedPlacement &p_placement) const;

	public:
		PlanChunkProvider(const Registries &p_registries, const WorldPlan &p_plan);

		void fill(spk::VoxelChunk &p_chunk) const;

		// Surface y of the column (for spawn placement); world coordinates.
		[[nodiscard]] int surfaceHeight(int p_worldX, int p_worldZ) const;
		[[nodiscard]] int maxHeight() const noexcept;
		// A road cell near the first gym: open ground, always connected to the network.
		[[nodiscard]] spk::Vector3Int spawnCell() const;
	};
}
