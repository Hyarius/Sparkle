#pragma once

#include "board/board_cell.hpp"
#include "board/board_occupancy.hpp"
#include "board/cell_source.hpp"
#include "board/deployment_layout.hpp"
#include "board/traversal_graph.hpp"
#include "board/traversal_graph_builder.hpp"
#include "board/weighted_path.hpp"

#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	class VoxelWorld;

	struct BoardExtent
	{
		// x is the X extent, y the Z extent (Vector2Int has no z). Both strictly positive.
		spk::Vector2Int size{};
		TraversalBounds traversalBounds{};
	};

	// One chunk the live board was built from, and the exact content revision it held then.
	struct RequiredChunkStamp
	{
		spk::Vector3Int coordinates{};
		std::uint64_t contentRevision = 0;

		[[nodiscard]] bool operator==(const RequiredChunkStamp &) const = default;
	};

	// The identity of the live terrain a board froze. It is deliberately NOT VoxelWorld::revision()
	// or VoxelMap::revision(): those move whenever any chunk anywhere loads, unloads or is edited,
	// so a streamer loading a chunk a hundred cells away would look like the battlefield changing.
	// A per-required-chunk content revision only moves when terrain the board actually read moves.
	struct LiveBoardTerrainStamp
	{
		const VoxelWorld *world = nullptr;      // borrowed; the map lifetime token below guards it
		std::weak_ptr<const void> mapLifetime;
		std::vector<RequiredChunkStamp> requiredChunks; // coordinate sorted
	};

	enum class BoardBuildErrorCode
	{
		InvalidRequest,
		NumericOverflow,
		RequiredChunkUnavailable,
		NoFeasibleWorldAnchor,
		InvalidHandcraftedDefinition,
		PrefabVoxelOutOfBounds,
		TraversalBuildFailed,
		InsufficientDeploymentCapacity
	};

	// An expected failure, as a value. Building a board over terrain nobody can stand on, or over a
	// chunk the streamer took away, is content/timing - not a broken process - so it is reported,
	// not thrown, and the lifecycle may log it and decline the encounter. Exceptions stay reserved
	// for a violated invariant.
	struct BoardBuildError
	{
		BoardBuildErrorCode code = BoardBuildErrorCode::InvalidRequest;
		std::string boardOrEncounterId;
		std::optional<std::string> prefabId;
		std::optional<spk::Vector3Int> cell;
		std::size_t requiredPlayerSeats = 0;
		std::size_t availablePlayerSeats = 0;
		std::size_t requiredEnemySeats = 0;
		std::size_t availableEnemySeats = 0;
		std::string diagnosticDetail;
	};

	[[nodiscard]] std::string_view toString(BoardBuildErrorCode p_code) noexcept;

	class BoardMutation;

	// The one terrain/navigation/occupancy surface a battle rule ever receives. Later steps read
	// standability, cost, borders, deployment, occupancy and line of sight from here and never from
	// VoxelWorld, a VoxelChunk, a render entity or WorldNavigation.
	//
	// Ownership: the board owns its cell source, graph, layout and occupancy. A live source borrows
	// the VoxelWorld and the registries, which must outlive the board - step 12 destroys the session
	// and this board before it unpins or replaces that world. A handcrafted or synthetic source owns
	// (through a shared_ptr) the immutable grid it reads, so it survives every construction temporary
	// and depends on no chunk, stream or seed.
	//
	// The graph is built exactly once, at construction, and is never refreshed while the battle
	// runs. Terrain that moved under a live board is a controlled abort (terrainIsCurrent() /
	// requireCurrentTerrain()), never a silent rebuild.
	class BoardData
	{
		friend class BoardMutation;

	public:
		// Every part of a board, so the constructor can check the closed source invariants instead
		// of trusting a long argument list.
		struct Parts
		{
			BoardSourceDescriptor source;
			std::shared_ptr<const ICellSource> cells;
			spk::Vector3Int presentationOrigin{};
			std::optional<spk::Vector3Int> liveWorldAnchor;
			BoardExtent extent;
			TraversalGraph navigation;
			DeploymentLayout deployment;
			std::vector<BoardCell> borderCells;
			std::optional<LiveBoardTerrainStamp> liveTerrainStamp;
		};

	private:
		BoardSourceDescriptor _source;
		std::shared_ptr<const ICellSource> _cells;
		spk::Vector3Int _presentationOrigin{};
		std::optional<spk::Vector3Int> _liveWorldAnchor;
		BoardExtent _extent;
		TraversalGraph _navigation;
		BoardOccupancy _occupancy;
		DeploymentLayout _deployment;
		std::vector<BoardCell> _borderCells;
		std::optional<LiveBoardTerrainStamp> _liveTerrainStamp;

		[[nodiscard]] std::string _describe(const BoardCell &p_local) const;

	public:
		// Throws std::invalid_argument when the parts break a closed invariant: a LiveWorld board
		// needs an anchor, a terrain stamp and no definition id, and its presentation origin IS that
		// anchor; a Handcrafted board needs a definition id, forbids both live values, and has a zero
		// presentation origin.
		explicit BoardData(Parts p_parts);

		[[nodiscard]] const BoardSourceDescriptor &sourceDescriptor() const noexcept;
		[[nodiscard]] const ICellSource &cells() const noexcept;
		[[nodiscard]] const TraversalGraph &navigation() const noexcept;
		[[nodiscard]] const BoardOccupancy &occupancy() const noexcept;
		[[nodiscard]] const DeploymentLayout &deployment() const noexcept;
		[[nodiscard]] const BoardExtent &extent() const noexcept;
		[[nodiscard]] const spk::Vector3Int &presentationOrigin() const noexcept;
		[[nodiscard]] const std::optional<spk::Vector3Int> &liveWorldAnchor() const noexcept;
		[[nodiscard]] const std::optional<LiveBoardTerrainStamp> &liveTerrainStamp() const noexcept;
		[[nodiscard]] const std::vector<BoardCell> &borderCells() const noexcept;

		[[nodiscard]] bool isInsideColumn(int p_x, int p_z) const noexcept;
		[[nodiscard]] bool isInside(const BoardCell &p_local) const noexcept;
		// The graph holds exactly this support cell. Standability is never inferred from the column:
		// two standable cells may share one x/z (a bridge over a floor).
		[[nodiscard]] bool isStandable(const BoardCell &p_local) const noexcept;
		[[nodiscard]] bool isBorderCell(const BoardCell &p_local) const noexcept;

		// What entering this cell costs: the authored movementCost of the solid support voxel under
		// the unit. Throws when the cell is not a current standable node with a resolved definition -
		// a broken registry or a stale source is a bug, and returning 1 would hide it.
		[[nodiscard]] int movementCost(const BoardCell &p_local) const;

		// The presentation space both sources share: local + presentationOrigin, which is the world
		// anchor for a live board and zero for a handcrafted one. Picking and rendering use this pair
		// rather than pretending every board belongs to the exploration world.
		[[nodiscard]] spk::Vector3Int toPresentationCell(const BoardCell &p_local) const;
		// nullopt on checked-arithmetic failure, outside the extent/traversal bounds, or absent from
		// the frozen topology. It never clamps and never searches downward for a nearby floor.
		[[nodiscard]] std::optional<BoardCell> fromPresentationCell(const spk::Vector3Int &p_presentation) const;

		// Live-only, and fatal on misuse: a handcrafted arena has no world coordinates, and its zero
		// presentation origin must never let it pass as live terrain. Throws std::logic_error on a
		// non-live board and std::overflow_error on checked-arithmetic failure; a caller accepting an
		// externally supplied world cell tests isInside/isStandable afterwards.
		[[nodiscard]] spk::Vector3Int toLiveWorldCell(const BoardCell &p_local) const;
		[[nodiscard]] BoardCell fromLiveWorldCell(const spk::Vector3Int &p_world) const;

		// Where a unit standing on this cell is drawn. walkHeightAtCenter already returns the top
		// walking height, so nothing adds another +1 on top of it.
		[[nodiscard]] spk::Vector3 unitPresentationPosition(const BoardCell &p_local) const;

		// Live: the map still exists and every required chunk is still loaded, still active and still
		// at the content revision the graph was built against. Handcrafted/synthetic: always true, by
		// construction - the backing grid is immutable and belongs to the board.
		[[nodiscard]] bool terrainIsCurrent() const noexcept;
		// Throws std::runtime_error when it is not. A required chunk that vanished, deactivated or
		// changed is a controlled battle abort, never a silent graph rebuild.
		void requireCurrentTerrain() const;
	};

	// Objects that stop a unit from entering a cell. Step 10 implements this over its battle-object
	// definitions; board code only asks the question.
	class IBoardMovementBlockers
	{
	public:
		virtual ~IBoardMovementBlockers() = default;
		[[nodiscard]] virtual bool blocksMovement(const BoardCell &p_cell) const noexcept = 0;
	};

	// The movement query every battle path and flood uses:
	//   blocked when another unit stands there, or a step-10 blocking object does;
	//   otherwise the destination cell's own terrain cost.
	// The moving unit never blocks itself, and there is no ally pass-through in v1.
	//
	// The returned query borrows the board and the blockers: both must outlive it.
	[[nodiscard]] TraversalCostQuery boardMovementQuery(
		const BoardData &p_board,
		BattleUnitId p_mover,
		const IBoardMovementBlockers *p_blockers = nullptr);

	// The writable handle onto occupancy. Step 06 hands it to BattleContext and the command
	// resolver; everyone else takes BoardData by const reference and can only read. Holding one is
	// not permission to bypass the single command path.
	class BoardMutation
	{
	private:
		BoardData &_board;

	public:
		explicit BoardMutation(BoardData &p_board) noexcept;

		[[nodiscard]] BoardMutationResult placeUnit(BattleUnitId p_unit, BoardCell p_cell) const;
		[[nodiscard]] BoardMutationResult moveUnit(BattleUnitId p_unit, BoardCell p_destination) const;
		[[nodiscard]] BoardMutationResult swapUnits(BattleUnitId p_first, BattleUnitId p_second) const;
		[[nodiscard]] bool removeUnit(BattleUnitId p_unit) const noexcept;
		[[nodiscard]] BoardMutationResult placeObject(BattleObjectId p_object, BoardCell p_cell) const;
		[[nodiscard]] bool removeObject(BattleObjectId p_object) const noexcept;
	};

	// Exactly one of the two is populated.
	struct BoardBuildResult
	{
		std::optional<BoardData> board;
		std::optional<BoardBuildError> error;
	};
}
