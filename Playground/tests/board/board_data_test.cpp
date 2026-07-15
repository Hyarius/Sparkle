#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_builder.hpp"
#include "board/board_data.hpp"

#include <memory>
#include <stdexcept>

namespace
{
	// A 5x5 field: cost-1 ground, one cost-2 mud cell, one cost-3 deep-mud cell, and a hole at
	// (3, *, 3) nobody can stand in.
	[[nodiscard]] pgtest::BoardFixture::Request costedField()
	{
		return pgtest::BoardFixture::Request{
			.layers =
				{"#####\n"
				 "#2###\n"
				 "###3#\n"
				 "###.#\n"
				 "#####",
				 ".....\n"
				 ".....\n"
				 ".....\n"
				 ".....\n"
				 ".....",
				 ".....\n"
				 ".....\n"
				 ".....\n"
				 ".....\n"
				 "....."},
			.deploymentDepth = 1,
			.namedCells = {{"player", pg::BoardCell{2, 0, 0}}, {"enemy", pg::BoardCell{2, 0, 4}}}};
	}
}

TEST(BoardDataTest, AHandcraftedBoardIsPresentedAtItsOwnOriginAndHasNoWorldCoordinates)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5));
	const pg::BoardData &board = fixture.board();

	EXPECT_EQ(board.sourceDescriptor().kind, pg::BoardSourceKind::Handcrafted);
	ASSERT_TRUE(board.sourceDescriptor().definitionId.has_value());
	EXPECT_EQ(*board.sourceDescriptor().definitionId, "test-flat-board");
	EXPECT_EQ(board.presentationOrigin(), spk::Vector3Int(0, 0, 0));
	EXPECT_FALSE(board.liveWorldAnchor().has_value());

	// Presentation is the identity here, and it is still the presentation seam - not an excuse to
	// treat the arena as if it lived in the world.
	const pg::BoardCell local{2, 0, 3};
	EXPECT_EQ(board.toPresentationCell(local), spk::Vector3Int(2, 0, 3));
	EXPECT_EQ(board.fromPresentationCell({2, 0, 3}), std::optional<pg::BoardCell>(local));

	EXPECT_THROW(auto value = board.toLiveWorldCell(local), std::logic_error);
	EXPECT_THROW(auto value = board.fromLiveWorldCell({2, 0, 3}), std::logic_error);
}

TEST(BoardDataTest, PresentationRoundTripsEveryTopologyCellAndRejectsTheRestWithoutClamping)
{
	pgtest::BoardFixture fixture(costedField());
	const pg::BoardData &board = fixture.board();

	for (const pg::TraversalGraph::Node &node : board.navigation().allNodes())
	{
		const std::optional<pg::BoardCell> back = board.fromPresentationCell(board.toPresentationCell(node.position));
		ASSERT_TRUE(back.has_value()) << node.position.toString();
		EXPECT_EQ(*back, node.position);
	}

	// The hole is inside the box and absent from the topology: it maps back to nothing rather than
	// to the nearest floor. Nothing clamps, nothing searches downward.
	EXPECT_FALSE(board.isStandable({3, 0, 3}));
	EXPECT_EQ(board.fromPresentationCell({3, 0, 3}), std::nullopt);
	// Outside the extent entirely.
	EXPECT_EQ(board.fromPresentationCell({-1, 0, 0}), std::nullopt);
	EXPECT_EQ(board.fromPresentationCell({5, 0, 0}), std::nullopt);
	// Inside the column, above the ground: air is not a support cell.
	EXPECT_EQ(board.fromPresentationCell({0, 1, 0}), std::nullopt);
}

TEST(BoardDataTest, StandabilityIsTheGraphAndBordersAreItsStandableRing)
{
	pgtest::BoardFixture fixture(costedField());
	const pg::BoardData &board = fixture.board();

	EXPECT_TRUE(board.isInside({3, 0, 3}));
	EXPECT_FALSE(board.isStandable({3, 0, 3})) << "an air column is inside the board and still unstandable";
	EXPECT_FALSE(board.isInside({0, 3, 0})) << "above the declared height";
	EXPECT_TRUE(board.isStandable({0, 0, 0}));

	EXPECT_TRUE(board.isBorderCell({0, 0, 0}));
	EXPECT_TRUE(board.isBorderCell({4, 0, 2}));
	EXPECT_FALSE(board.isBorderCell({2, 0, 2}));
	// Sorted, and only standable cells.
	EXPECT_TRUE(std::ranges::is_sorted(board.borderCells(), pg::BoardCellLess{}));
	for (const pg::BoardCell &cell : board.borderCells())
	{
		EXPECT_TRUE(board.isStandable(cell)) << cell.toString();
	}
}

TEST(BoardDataTest, TerrainMovementCostIsAuthoredDataAndOneByDefault)
{
	pgtest::BoardFixture fixture(costedField());
	const pg::BoardData &board = fixture.board();

	EXPECT_EQ(board.movementCost({0, 0, 0}), 1) << "the default cost of a voxel that authors none";
	EXPECT_EQ(board.movementCost({1, 0, 1}), 2);
	EXPECT_EQ(board.movementCost({3, 0, 2}), 3);

	// Not a cell anyone can stand on: this throws rather than quietly answering 1.
	EXPECT_THROW(auto value = board.movementCost({3, 0, 3}), std::runtime_error);
	EXPECT_THROW(auto value = board.movementCost({0, 1, 0}), std::runtime_error);
}

TEST(BoardDataTest, HandcraftedTerrainIsAlwaysCurrentAndKeepsItsGridAlive)
{
	// The fixture's construction temporaries are long gone by the time the board is read: the grid
	// is owned by the source, not borrowed from a stack.
	std::unique_ptr<pgtest::BoardFixture> fixture =
		std::make_unique<pgtest::BoardFixture>(pgtest::BoardFixture::flat(5, 5));
	const pg::BoardData &board = fixture->board();

	EXPECT_TRUE(board.terrainIsCurrent());
	EXPECT_NO_THROW(board.requireCurrentTerrain());
	EXPECT_FALSE(board.liveTerrainStamp().has_value());
	// It still reads its cells, with no world, chunk, streamer, fluid or seed anywhere in sight.
	EXPECT_EQ(board.movementCost({0, 0, 0}), 1);
}

TEST(BoardDataTest, WalkingHeightFollowsTheSupportVoxelShape)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::Request{
		.layers =
			{"#_/s#\n"
			 "#####\n"
			 "#####\n"
			 "#####\n"
			 "#####",
			 ".....\n"
			 ".....\n"
			 ".....\n"
			 ".....\n"
			 ".....",
			 ".....\n"
			 ".....\n"
			 ".....\n"
			 ".....\n"
			 "....."},
		.deploymentDepth = 1});
	const pg::BoardData &board = fixture.board();

	// A unit's feet sit on the top walking height of its support voxel, and nothing adds a second +1
	// on top of that: a full cube is 1.0 above its own Y, a slab 0.5.
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({0, 0, 0}).y, 1.0f) << "cube";
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({1, 0, 0}).y, 0.5f) << "slab";
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({2, 0, 0}).y, 0.5f) << "slope, at its centre";
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({3, 0, 0}).y, 0.5f) << "stair, at its centre";

	// And it is drawn in the middle of its cell.
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({0, 0, 0}).x, 0.5f);
	EXPECT_FLOAT_EQ(board.unitPresentationPosition({0, 0, 0}).z, 0.5f);
}

TEST(BoardDataTest, TheSourceDescriptorInvariantsAreClosed)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(5, 5));
	const pg::BoardData &board = fixture.board();

	// A copy of the descriptor is a value later snapshots and digests can keep: it outlives the
	// board and compares equal.
	const pg::BoardSourceDescriptor copied = board.sourceDescriptor();
	EXPECT_EQ(copied, board.sourceDescriptor());
	EXPECT_NE(
		copied,
		(pg::BoardSourceDescriptor{.kind = pg::BoardSourceKind::LiveWorld, .definitionId = std::nullopt}));

	// A handcrafted board without an id, or a live one carrying one, is rejected at construction: an
	// arena may never masquerade as live terrain, whatever its presentation origin says.
	pg::BoardData::Parts parts{
		.source = {.kind = pg::BoardSourceKind::Handcrafted, .definitionId = std::nullopt},
		.cells = std::make_shared<const pg::GridCellSource>(
			pgtest::gridFromLayers({"###\n###\n###"}, fixture.voxels()), fixture.voxels()),
		.presentationOrigin = {},
		.liveWorldAnchor = std::nullopt,
		.extent = {.size = {3, 3}, .traversalBounds = {.minimum = {0, 0, 0}, .maximum = {3, 1, 3}}},
		.navigation = {},
		.deployment = {},
		.borderCells = {},
		.liveTerrainStamp = std::nullopt};
	EXPECT_THROW(pg::BoardData rejected(parts), std::invalid_argument) << "handcrafted, with no definition id";

	parts.source = {.kind = pg::BoardSourceKind::LiveWorld, .definitionId = "sneaky-arena"};
	EXPECT_THROW(pg::BoardData rejected(parts), std::invalid_argument) << "live, carrying a definition id";

	parts.source = {.kind = pg::BoardSourceKind::LiveWorld, .definitionId = std::nullopt};
	EXPECT_THROW(pg::BoardData rejected(parts), std::invalid_argument) << "live, with no anchor and no stamp";
}
