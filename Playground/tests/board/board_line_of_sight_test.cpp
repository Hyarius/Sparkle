#include <gtest/gtest.h>

#include "fixtures/board_fixture.hpp"

#include "board/board_line_of_sight.hpp"

#include <set>
#include <stdexcept>

namespace
{
	// A 7x5 field. The ground is at y = 0, so an eye sits at 1.0 + 0.65 and every sight line runs
	// through the y = 1 layer - which is where these cases put their walls.
	//
	//   z=0  clear                     z=1  a passable bush at x = 3
	//   z=2  a solid wall at x = 3     z=3  a solid losTransparent pane at x = 3
	//   z=4  two walls, at x = 2 and x = 4
	[[nodiscard]] pgtest::BoardFixture::Request obstacleField()
	{
		return pgtest::BoardFixture::Request{
			.layers =
				{"#######\n#######\n#######\n#######\n#######",
				 ".......\n"
				 "...b...\n"
				 "...W...\n"
				 "...G...\n"
				 "..W.W..",
				 ".......\n.......\n.......\n.......\n......."},
			.deploymentDepth = 1};
	}

	// Blocks exactly the voxels it was handed. Step 10 will map each blocksLineOfSight object to the
	// voxel above its support cell; this stands in for that.
	class FixedBlockers : public pg::IBoardLineOfSightExtraBlockers
	{
	private:
		std::set<spk::Vector3Int, pg::CellPositionLess> _blocked;

	public:
		explicit FixedBlockers(std::set<spk::Vector3Int, pg::CellPositionLess> p_blocked) :
			_blocked(std::move(p_blocked))
		{
		}

		[[nodiscard]] bool blocksVoxel(const spk::Vector3Int &p_voxel) const noexcept override
		{
			return _blocked.contains(p_voxel);
		}
	};
}

TEST(BoardLineOfSightTest, OnlyOpaqueSolidVoxelsBlock)
{
	pgtest::BoardFixture fixture(obstacleField());
	const pg::BoardData &board = fixture.board();

	const pg::LineOfSightResult clear = pg::BoardLineOfSight::trace(board, {0, 0, 0}, {6, 0, 0});
	EXPECT_TRUE(clear.clear);
	EXPECT_FALSE(clear.firstBlockingVoxel.has_value());

	// A bush is passable, so a unit sights straight through it - as it walks through it.
	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, {0, 0, 1}, {6, 0, 1}).clear);

	// A wall is solid and opaque, and it names the voxel that stopped the line.
	const pg::LineOfSightResult blocked = pg::BoardLineOfSight::trace(board, {0, 0, 2}, {6, 0, 2});
	EXPECT_FALSE(blocked.clear);
	EXPECT_EQ(blocked.firstBlockingVoxel, std::optional<spk::Vector3Int>(spk::Vector3Int{3, 1, 2}));

	// Solid and see-through: it stops a unit, never a sight line. Sight is a gameplay tag, not a
	// render transparency and not a physics answer.
	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, {0, 0, 3}, {6, 0, 3}).clear);

	// Reversing the endpoints reverses nothing about whether the shot is on.
	EXPECT_FALSE(pg::BoardLineOfSight::trace(board, {6, 0, 2}, {0, 0, 2}).clear);
	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, {6, 0, 3}, {0, 0, 3}).clear);
}

TEST(BoardLineOfSightTest, TheFirstBlockerInTraversalOrderIsTheOneReported)
{
	pgtest::BoardFixture fixture(obstacleField());
	const pg::BoardData &board = fixture.board();

	const pg::LineOfSightResult fromWest = pg::BoardLineOfSight::trace(board, {0, 0, 4}, {6, 0, 4});
	EXPECT_FALSE(fromWest.clear);
	EXPECT_EQ(fromWest.firstBlockingVoxel, std::optional<spk::Vector3Int>(spk::Vector3Int{2, 1, 4}));

	// From the other end, the other wall comes first: the answer is the same, the witness is not.
	const pg::LineOfSightResult fromEast = pg::BoardLineOfSight::trace(board, {6, 0, 4}, {0, 0, 4});
	EXPECT_FALSE(fromEast.clear);
	EXPECT_EQ(fromEast.firstBlockingVoxel, std::optional<spk::Vector3Int>(spk::Vector3Int{4, 1, 4}));
}

TEST(BoardLineOfSightTest, AnElevatedWallDoesNotBlockAnEyeLevelLine)
{
	// The same wall, one layer higher: the eyes look under it. Elevation is part of the query, not an
	// afterthought - which is exactly why range is x/z and sight is 3D.
	pgtest::BoardFixture fixture(pgtest::BoardFixture::Request{
		.layers =
			{"#####\n#####\n#####",
			 ".....\n.....\n.....",
			 "..W..\n..W..\n..W..",
			 ".....\n.....\n....."},
		.deploymentDepth = 1});

	EXPECT_TRUE(pg::BoardLineOfSight::trace(fixture.board(), {0, 0, 1}, {4, 0, 1}).clear);
}

TEST(BoardLineOfSightTest, ADiagonalCannotLeakThroughACorner)
{
	// Two walls meeting exactly on the diagonal. A naive DDA would slip between them at the corner
	// point; the supercover traversal inspects both newly touched cells first, in canonical X/Y/Z
	// order, so the shot is blocked and the answer is reproducible.
	pgtest::BoardFixture fixture(pgtest::BoardFixture::Request{
		.layers = {"###\n###\n###", ".W.\nW..\n...", "...\n...\n..."},
		.deploymentDepth = 1});
	const pg::BoardData &board = fixture.board();

	const pg::LineOfSightResult diagonal = pg::BoardLineOfSight::trace(board, {0, 0, 0}, {2, 0, 2});
	EXPECT_FALSE(diagonal.clear);
	EXPECT_EQ(diagonal.firstBlockingVoxel, std::optional<spk::Vector3Int>(spk::Vector3Int{1, 1, 0}))
		<< "the X-step cell is inspected before the Z-step one and before the corner itself";

	// And it is still blocked the other way round.
	EXPECT_FALSE(pg::BoardLineOfSight::trace(board, {2, 0, 2}, {0, 0, 0}).clear);
}

TEST(BoardLineOfSightTest, NeitherEndpointColumnBlindsItself)
{
	pgtest::BoardFixture fixture(pgtest::BoardFixture::flat(7, 3));
	const pg::BoardData &board = fixture.board();
	const pg::BoardCell caster{0, 0, 1};
	const pg::BoardCell targetCell{6, 0, 1};

	// A step-10 blocker standing on the caster's own cell, and one on the target's: both map to the
	// voxel above their support, and both are inside an endpoint column. A unit does not blind itself
	// with the object it is standing on, nor with the one its target stands on.
	const FixedBlockers ownColumns({{0, 1, 1}, {6, 1, 1}});
	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, caster, targetCell, &ownColumns).clear);

	// One cell further along, in nobody's column, the very same blocker stops the line.
	const FixedBlockers between({{3, 1, 1}});
	const pg::LineOfSightResult blocked = pg::BoardLineOfSight::trace(board, caster, targetCell, &between);
	EXPECT_FALSE(blocked.clear);
	EXPECT_EQ(blocked.firstBlockingVoxel, std::optional<spk::Vector3Int>(spk::Vector3Int{3, 1, 1}));

	// Several blockers in one voxel are still one obstruction, and an empty extra query changes
	// nothing about the terrain answer.
	const FixedBlockers none({});
	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, caster, targetCell, &none).clear);
}

TEST(BoardLineOfSightTest, ASlopeAndAStairAreSightedFromTheirWalkingHeight)
{
	// The caster stands on a slope (walk height 0.5) and the target on a stair; both eyes sit above
	// their own support, so neither is blinded by the shape it is standing on.
	pgtest::BoardFixture fixture(pgtest::BoardFixture::Request{
		.layers = {"/###s\n#####\n#####", ".....\n..W..\n.....", ".....\n.....\n....."},
		.deploymentDepth = 1});
	const pg::BoardData &board = fixture.board();

	EXPECT_TRUE(pg::BoardLineOfSight::trace(board, {0, 0, 0}, {4, 0, 0}).clear);
	// And the wall one row down still blocks the line that crosses it.
	EXPECT_FALSE(pg::BoardLineOfSight::trace(board, {0, 0, 1}, {4, 0, 1}).clear);
}

TEST(BoardLineOfSightTest, EndpointsMustBeStandableCellsOfThisBoard)
{
	pgtest::BoardFixture fixture(obstacleField());
	const pg::BoardData &board = fixture.board();

	// A unit standing on itself sees itself.
	const pg::LineOfSightResult itself = pg::BoardLineOfSight::trace(board, {0, 0, 0}, {0, 0, 0});
	EXPECT_TRUE(itself.clear);

	// Air, a wall's own cell, and a coordinate off the board are all rejected rather than answered.
	EXPECT_THROW(auto value = pg::BoardLineOfSight::trace(board, {0, 1, 0}, {6, 0, 0}), std::invalid_argument);
	EXPECT_THROW(auto value = pg::BoardLineOfSight::trace(board, {0, 0, 0}, {3, 0, 2}), std::invalid_argument)
		<< "the cell under the wall is not standable";
	EXPECT_THROW(auto value = pg::BoardLineOfSight::trace(board, {0, 0, 0}, {99, 0, 0}), std::invalid_argument);
}
