#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_builder.hpp"
#include "core/paths.hpp"
#include "voxel/voxel_registry.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <limits>
#include <memory>
#include <vector>

namespace
{
	// A frozen minimal live world, owned by the test: a real VoxelWorld read through the real
	// WorldCellSource. A board claiming LiveWorld provenance is never an owned grid wearing a label.
	class TestWorld
	{
	private:
		pg::ShapeCatalog _shapes;
		pg::VoxelFamilyCatalog _families;
		pg::VoxelRegistry _voxels;
		std::unique_ptr<pg::VoxelWorld> _world;

	public:
		// Ground at world y = 0 wherever p_ground says so, air above it: a flat field with an
		// optional cliff, and nothing else.
		explicit TestWorld(std::function<bool(int, int)> p_ground = [](int, int) {
			return true;
		})
		{
			pgtest::loadFixtureVoxels(_shapes, _families, _voxels);
			const spk::VoxelRuntimeId ground = _voxels.runtimeId("test-ground");
			_world = std::make_unique<pg::VoxelWorld>(
				_voxels, [ground, p_ground = std::move(p_ground)](spk::VoxelChunk &p_chunk) {
					const spk::Vector3Int origin = p_chunk.worldOrigin();
					p_chunk.editCells([&](spk::VoxelChunk::Editor &p_editor) {
						for (int z = 0; z < spk::VoxelChunk::Size.z; ++z)
						{
							for (int x = 0; x < spk::VoxelChunk::Size.x; ++x)
							{
								const int worldX = origin.x + x;
								const int worldZ = origin.z + z;
								if (origin.y == 0 && p_ground(worldX, worldZ))
								{
									(void)p_editor.setCell({x, 0, z}, spk::VoxelCell{.id = ground});
								}
							}
						}
					});
				});
		}

		[[nodiscard]] pg::VoxelWorld &world() noexcept
		{
			return *_world;
		}
		[[nodiscard]] const pg::VoxelRegistry &voxels() const noexcept
		{
			return _voxels;
		}

		void load(const pg::WorldBoardPlan &p_plan)
		{
			for (const spk::Vector3Int &coordinates : p_plan.requiredChunkCoordinates)
			{
				_world->loadChunk(coordinates);
			}
		}
	};

	[[nodiscard]] pg::WorldBoardRequest request(
		const spk::Vector3Int &p_encounter,
		const spk::Vector2Int &p_size = {5, 5},
		std::size_t p_playerSeats = 1,
		std::size_t p_enemySeats = 1)
	{
		return pg::WorldBoardRequest{
			.encounterSupportCell = p_encounter,
			.size = p_size,
			.minimumWorldY = 0,
			.maximumWorldY = 1,
			.approachDirection = pg::VoxelOrientation::PositiveZ,
			.deploymentDepth = 1,
			.playerSeatCount = p_playerSeats,
			.enemySeatCount = p_enemySeats};
	}
}

TEST(BoardBuilderTest, TheAnchorIsCentredOnTheEncounterAndTheCandidatesFollowTheLockedOrder)
{
	// An odd board has a centre column; an even one anchors half a cell "before" the encounter.
	const pg::WorldBoardPlanResult odd = pg::BoardBuilder::planWorld(request({10, 0, 20}, {11, 11}));
	ASSERT_TRUE(odd.plan.has_value());
	EXPECT_EQ(odd.plan->candidateWorldAnchors.front(), spk::Vector3Int(5, 0, 15));

	const pg::WorldBoardPlanResult even = pg::BoardBuilder::planWorld(request({10, 0, 20}, {6, 6}));
	ASSERT_TRUE(even.plan.has_value());
	EXPECT_EQ(even.plan->candidateWorldAnchors.front(), spk::Vector3Int(7, 0, 17));

	// The exact nine, in the exact order: the primary, the four axis nudges, then the four diagonals.
	EXPECT_EQ(
		odd.plan->candidateWorldAnchors,
		(std::vector<spk::Vector3Int>{
			{5, 0, 15},
			{7, 0, 15},
			{3, 0, 15},
			{5, 0, 17},
			{5, 0, 13},
			{7, 0, 17},
			{3, 0, 17},
			{7, 0, 13},
			{3, 0, 13}}));

	// The anchor never rebases Y: local Y stays world Y, so slopes, stairs and voxel DDA keep meaning
	// the same thing in a battle as they do in exploration.
	for (const spk::Vector3Int &anchor : odd.plan->candidateWorldAnchors)
	{
		EXPECT_EQ(anchor.y, 0);
	}
}

TEST(BoardBuilderTest, ThePinRegionCoversEveryCandidateAndTheHeadClearanceAboveIt)
{
	// A board straddling the chunk origin, so the required set spans negative chunk coordinates, and
	// a Y range whose head clearance (two cells above the top scanned support cell) reaches into the
	// chunk above.
	pg::WorldBoardRequest wide = request({0, 15, 0}, {11, 11});
	wide.minimumWorldY = 0;
	wide.maximumWorldY = 16;

	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(wide);
	ASSERT_TRUE(planned.plan.has_value());
	const pg::WorldBoardPlan &plan = *planned.plan;

	// Sorted by x, y, z and deduplicated: the nine candidate rectangles overlap heavily, and a chunk
	// is required once.
	EXPECT_TRUE(std::ranges::is_sorted(plan.requiredChunkCoordinates, pg::CellPositionLess{}));
	EXPECT_EQ(
		std::ranges::adjacent_find(plan.requiredChunkCoordinates),
		plan.requiredChunkCoordinates.end())
		<< "no chunk is required twice";

	// The candidates span x/z in [-7, 9], so chunks -1 and 0 on both axes. The scan reaches y = 15 and
	// the clearance reaches y = 17, so chunks 0 and 1 vertically.
	EXPECT_EQ(plan.pinWindowOriginChunk, spk::Vector3Int(-1, 0, -1));
	EXPECT_EQ(plan.pinWindowRange, spk::Vector3Int(2, 2, 2));
	EXPECT_EQ(plan.requiredChunkCoordinates.size(), 8U);
	EXPECT_NE(
		std::ranges::find(plan.requiredChunkCoordinates, spk::Vector3Int(0, 1, 0)),
		plan.requiredChunkCoordinates.end())
		<< "the head-clearance chunk above the board is required too";
}

TEST(BoardBuilderTest, AnInvalidRequestOrAWrappingCoordinateIsRejectedAsAValue)
{
	EXPECT_EQ(
		pg::BoardBuilder::planWorld(request({0, 0, 0}, {4, 5})).error->code,
		pg::BoardBuildErrorCode::InvalidRequest)
		<< "below the minimum board side";

	pg::WorldBoardRequest inverted = request({0, 0, 0});
	inverted.minimumWorldY = 4;
	inverted.maximumWorldY = 4;
	EXPECT_EQ(pg::BoardBuilder::planWorld(inverted).error->code, pg::BoardBuildErrorCode::InvalidRequest);

	pg::WorldBoardRequest outside = request({0, 9, 0});
	EXPECT_EQ(pg::BoardBuilder::planWorld(outside).error->code, pg::BoardBuildErrorCode::InvalidRequest)
		<< "the encounter cell is outside the navigation bounds that made it reachable";

	pg::WorldBoardRequest deep = request({0, 0, 0});
	deep.deploymentDepth = 3;
	EXPECT_EQ(pg::BoardBuilder::planWorld(deep).error->code, pg::BoardBuildErrorCode::InvalidRequest)
		<< "two three-row strips do not fit in five rows";

	// The primary anchor of a board centred here would wrap the cell space.
	const pg::WorldBoardPlanResult wrapped =
		pg::BoardBuilder::planWorld(request({std::numeric_limits<int>::min() + 1, 0, 0}));
	ASSERT_TRUE(wrapped.error.has_value());
	EXPECT_EQ(wrapped.error->code, pg::BoardBuildErrorCode::NumericOverflow);
	EXPECT_FALSE(wrapped.plan.has_value());
}

TEST(BoardBuilderTest, TheBoardIsBuiltOverTheLiveWorldWithoutCopyingIt)
{
	TestWorld live;
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({20, 0, 20}));
	ASSERT_TRUE(planned.plan.has_value());
	live.load(*planned.plan);

	const pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	const pg::BoardData &board = *built.board;

	EXPECT_EQ(board.sourceDescriptor().kind, pg::BoardSourceKind::LiveWorld);
	EXPECT_FALSE(board.sourceDescriptor().definitionId.has_value());
	EXPECT_EQ(board.liveWorldAnchor(), std::optional<spk::Vector3Int>(spk::Vector3Int{18, 0, 18}));
	EXPECT_EQ(board.presentationOrigin(), spk::Vector3Int(18, 0, 18)) << "a live board is presented where it lives";

	// Board-local is the world, rebased on x/z only.
	EXPECT_EQ(board.toLiveWorldCell({0, 0, 0}), spk::Vector3Int(18, 0, 18));
	EXPECT_EQ(board.fromLiveWorldCell({20, 0, 20}), pg::BoardCell(2, 0, 2));
	EXPECT_EQ(board.toPresentationCell({2, 0, 2}), board.toLiveWorldCell({2, 0, 2}));
	EXPECT_EQ(board.movementCost({0, 0, 0}), 1);

	// It reads the very cells the world holds - nothing was copied into a private grid.
	EXPECT_EQ(board.cells().cell({2, 0, 2}).id, live.world().cell({20, 0, 20}).id);
	EXPECT_EQ(board.navigation().size(), 25U) << "five by five standable support cells";
	EXPECT_TRUE(board.terrainIsCurrent());
}

TEST(BoardBuilderTest, ALiveBoardRoundTripsNegativeWorldCoordinates)
{
	TestWorld live;
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({-20, 0, -20}));
	ASSERT_TRUE(planned.plan.has_value());
	live.load(*planned.plan);

	const pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	const pg::BoardData &board = *built.board;

	EXPECT_EQ(board.liveWorldAnchor(), std::optional<spk::Vector3Int>(spk::Vector3Int{-22, 0, -22}));
	for (const pg::TraversalGraph::Node &node : board.navigation().allNodes())
	{
		EXPECT_EQ(board.fromLiveWorldCell(board.toLiveWorldCell(node.position)), node.position);
		EXPECT_EQ(node.position.y, 0) << "local Y is world Y";
	}
}

TEST(BoardBuilderTest, TheFirstCandidateThatSeatsBothTeamsIsChosen)
{
	// A cliff: solid ground only from x = 0 eastward.
	TestWorld live([](int p_x, int) {
		return p_x >= 0;
	});
	// Centred on x = 0, the primary anchor is (-2): its five-cell rows only hold three standable
	// cells, so four creatures cannot deploy there. The next candidate, two cells east, can.
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({0, 0, 20}, {5, 5}, 4, 4));
	ASSERT_TRUE(planned.plan.has_value());
	live.load(*planned.plan);

	const pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	EXPECT_EQ(built.board->liveWorldAnchor(), std::optional<spk::Vector3Int>(spk::Vector3Int{0, 0, 18}))
		<< "the second candidate, not a shrunken board at the first";
	EXPECT_EQ(built.board->extent().size, spk::Vector2Int(5, 5)) << "and it is the board that was asked for";
	EXPECT_EQ(built.board->deployment().playerCells.size(), 5U);
}

TEST(BoardBuilderTest, NoFeasibleAnchorIsATypedRefusalRatherThanAShrunkenBoard)
{
	TestWorld live([](int p_x, int) {
		return p_x >= 0;
	});
	// Six creatures need six standable cells in a five-cell row: no nudge can fix that.
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({0, 0, 20}, {5, 5}, 6, 1));
	ASSERT_TRUE(planned.plan.has_value());
	live.load(*planned.plan);

	const pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_FALSE(built.board.has_value());
	ASSERT_TRUE(built.error.has_value());
	EXPECT_EQ(built.error->code, pg::BoardBuildErrorCode::NoFeasibleWorldAnchor);
	EXPECT_EQ(built.error->requiredPlayerSeats, 6U);
	EXPECT_EQ(built.error->availablePlayerSeats, 5U) << "the best any candidate could offer";
	EXPECT_FALSE(built.error->diagnosticDetail.empty()) << "and it says so per candidate";
}

TEST(BoardBuilderTest, AMissingOrInactiveRequiredChunkFailsBeforeAnyTerrainIsRead)
{
	TestWorld live;
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({20, 0, 20}));
	ASSERT_TRUE(planned.plan.has_value());

	// Nothing loaded yet: the region is not resident, so there is no terrain to build over.
	const pg::BoardBuildResult missing = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(missing.error.has_value());
	EXPECT_EQ(missing.error->code, pg::BoardBuildErrorCode::RequiredChunkUnavailable);
	EXPECT_TRUE(missing.error->cell.has_value()) << "and it names the chunk";

	live.load(*planned.plan);
	ASSERT_TRUE(live.world().map().setChunkActive(planned.plan->requiredChunkCoordinates.front(), false));
	const pg::BoardBuildResult inactive = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(inactive.error.has_value());
	EXPECT_EQ(inactive.error->code, pg::BoardBuildErrorCode::RequiredChunkUnavailable);
}

TEST(BoardBuilderTest, OnlyTheRequiredChunksStaleTheBoardAndNothingEverRebuildsIt)
{
	TestWorld live;
	const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({20, 0, 20}));
	ASSERT_TRUE(planned.plan.has_value());
	live.load(*planned.plan);

	pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
	ASSERT_TRUE(built.board.has_value()) << built.error->diagnosticDetail;
	pg::BoardData &board = *built.board;
	const std::size_t nodesAtBuild = board.navigation().size();

	// The streamer churning far away is not a gameplay event: an unrelated chunk loading, being
	// edited, and unloading again all move VoxelMap::revision(), and none of them touch this board.
	const spk::Vector3Int unrelated{40, 0, 40};
	live.world().loadChunk(unrelated);
	ASSERT_TRUE(live.world().setCell({640, 0, 640}, spk::VoxelCell{.id = live.voxels().runtimeId("test-wall")}));
	EXPECT_TRUE(board.terrainIsCurrent()) << "a chunk outside the required set is none of the board's business";
	ASSERT_TRUE(live.world().unloadChunk(unrelated));
	EXPECT_TRUE(board.terrainIsCurrent());
	EXPECT_NO_THROW(board.requireCurrentTerrain());

	// Editing terrain the board actually read is a controlled abort - never a silent rebuild.
	ASSERT_TRUE(live.world().setCell({20, 0, 20}, spk::VoxelCell{}));
	EXPECT_FALSE(board.terrainIsCurrent());
	EXPECT_THROW(board.requireCurrentTerrain(), std::runtime_error);
	EXPECT_THROW(auto value = board.movementCost({2, 0, 2}), std::runtime_error);
	EXPECT_EQ(board.navigation().size(), nodesAtBuild) << "the graph is exactly the one that was built once";
}

TEST(BoardBuilderTest, ARequiredChunkThatVanishesOrDeactivatesStalesTheBoard)
{
	const spk::Vector3Int required{1, 0, 1};

	{
		TestWorld live;
		const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({20, 0, 20}));
		ASSERT_TRUE(planned.plan.has_value());
		live.load(*planned.plan);
		pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
		ASSERT_TRUE(built.board.has_value());

		ASSERT_TRUE(live.world().map().setChunkActive(required, false));
		EXPECT_FALSE(built.board->terrainIsCurrent()) << "an inactive required chunk is not terrain to fight on";
	}
	{
		TestWorld live;
		const pg::WorldBoardPlanResult planned = pg::BoardBuilder::planWorld(request({20, 0, 20}));
		ASSERT_TRUE(planned.plan.has_value());
		live.load(*planned.plan);
		pg::BoardBuildResult built = pg::BoardBuilder::buildWorld(live.world(), *planned.plan, 0.5);
		ASSERT_TRUE(built.board.has_value());

		ASSERT_TRUE(live.world().unloadChunk(required));
		EXPECT_FALSE(built.board->terrainIsCurrent());
	}
}
