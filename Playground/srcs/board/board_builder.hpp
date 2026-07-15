#pragma once

#include "board/board_data.hpp"
#include "board/handcrafted_battle_board_definition.hpp"
#include "voxel/voxel_enums.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/prefab_definition.hpp"

#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace pg
{
	class VoxelWorld;

	// What the encounter asks for. The vertical bounds are not invented from encounter JSON: the
	// lifecycle copies them from the WorldNavigation that made the triggering support cell reachable,
	// so battle standability and exploration standability cannot disagree. X/Z still come from the
	// authored board size.
	struct WorldBoardRequest
	{
		spk::Vector3Int encounterSupportCell{};
		spk::Vector2Int size{};											  // x, z extents (Vector2Int::y is the Z extent)
		int minimumWorldY = 0;											  // inclusive support-cell scan bound
		int maximumWorldY = 0;											  // exclusive
		VoxelOrientation approachDirection = VoxelOrientation::PositiveZ; // horizontal only
		int deploymentDepth = 0;
		std::size_t playerSeatCount = 0;
		std::size_t enemySeatCount = 0;
	};

	// The read-free half of live construction: pure checked coordinate and chunk arithmetic, with no
	// cell ever inspected. Step 12 keeps this whole envelope loaded and active - and pauses the fluid
	// simulation - before anyone reads a voxel through it.
	struct WorldBoardPlan
	{
		WorldBoardRequest request;
		std::vector<spk::Vector3Int> candidateWorldAnchors;
		std::vector<spk::Vector3Int> requiredChunkCoordinates; // sorted by x,y,z, deduplicated
		spk::Vector3Int pinWindowOriginChunk{};
		spk::Vector3Int pinWindowRange{};
	};

	struct WorldBoardPlanResult
	{
		std::optional<WorldBoardPlan> plan;
		std::optional<BoardBuildError> error;
	};

	// Builds the one frozen board a battle runs on. Two production sources, one set of tactical rules:
	//
	//   live world  - plan, pin, pause, then build once over WorldCellSource. No voxel is copied,
	//                 stamped or restored (decision D03), and the graph is never refreshed while the
	//                 battle runs.
	//   handcrafted - materialise the referenced prefab at the identity transform into an owned
	//                 immutable grid. No world, no chunk, no streamer, no fluid, no seed, no RNG.
	//
	// Both funnel into buildFrozenSource, which alone builds the traversal graph, the deployment
	// zones, the border list and the capacity checks - so an arena and a field obey identical rules.
	class BoardBuilder
	{
	public:
		// Reads nothing. Rejects an invalid request or coordinate/chunk overflow as a value.
		[[nodiscard]] static WorldBoardPlanResult planWorld(const WorldBoardRequest &p_request);

		// Verifies every required chunk is loaded and active, picks the first candidate anchor whose
		// two deployment strips can seat both teams, and builds the graph exactly once for it. It
		// never shrinks the authored board, drops a creature, overlaps units or fabricates ground: a
		// board nobody can be deployed on is a typed failure the encounter may decline.
		[[nodiscard]] static BoardBuildResult buildWorld(
			const VoxelWorld &p_world,
			const WorldBoardPlan &p_plan,
			double p_maxVerticalGap);

		// The caller resolves both definitions from the already validated registries; construction is
		// still defensive and transactional. Every listed prefab voxel - carve entries included - is
		// validated against the grid BEFORE a single cell is written, so a prefab reaching outside its
		// arena rejects the board instead of being silently clipped by Prefab::applyTo.
		//
		// p_voxels is borrowed for the board's whole life (Registries outlives it); the grid is owned.
		[[nodiscard]] static BoardBuildResult buildHandcrafted(
			const HandcraftedBattleBoardDefinition &p_definition,
			const PrefabDefinition &p_geometry,
			const VoxelRegistry &p_voxels,
			std::size_t p_playerSeatCount,
			std::size_t p_enemySeatCount,
			double p_maxVerticalGap);

		// The shared tail of both, and the one the test fixture also calls with its own owned grid: a
		// fixture is not a third production source kind, it is a Handcrafted board with a stable test
		// definition id that no registry ever resolves.
		[[nodiscard]] static BoardBuildResult buildFrozenSource(
			BoardSourceDescriptor p_source,
			std::shared_ptr<const ICellSource> p_cells,
			BoardExtent p_extent,
			spk::Vector3Int p_presentationOrigin,
			std::optional<spk::Vector3Int> p_liveWorldAnchor,
			std::optional<LiveBoardTerrainStamp> p_liveTerrainStamp,
			VoxelOrientation p_playerApproach,
			int p_deploymentDepth,
			std::size_t p_playerSeatCount,
			std::size_t p_enemySeatCount,
			double p_maxVerticalGap);
	};
}
