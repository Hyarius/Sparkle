#include "battle/board_overlay_state.hpp"
#include "components/board_overlay_view.hpp"
#include "support/board_fixture.hpp"
#include "voxel/voxel_mesher.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace
{
	using pg::OverlayCategory;

	[[nodiscard]] std::array<pg::AtlasCell, pg::OverlayCategoryCount> flatMasks()
	{
		std::array<pg::AtlasCell, pg::OverlayCategoryCount> masks{};
		masks.fill(pg::AtlasCell{0, 0});
		return masks;
	}
}

TEST(BoardOverlayState, EditsBumpTheChangeCounterAndEmptyClearsDoNot)
{
	pg::BoardOverlayState state;
	const std::uint64_t start = state.changeCounter;

	state.set(OverlayCategory::MoveRange, {{0, 0, 0}, {1, 0, 0}});
	EXPECT_EQ(state.changeCounter, start + 1);
	EXPECT_EQ(state.cells(OverlayCategory::MoveRange).size(), 2u);

	state.clear(OverlayCategory::Deployment); // already empty -> no rebuild needed
	EXPECT_EQ(state.changeCounter, start + 1);

	state.clear(OverlayCategory::MoveRange); // had cells -> bump
	EXPECT_EQ(state.changeCounter, start + 2);

	state.clearAll();
	EXPECT_EQ(state.changeCounter, start + 3);
}

TEST(BoardOverlayView, RebuildsOncePerChangeCounterMove)
{
	pg::test::BoardFixture fixture({"###", "###", "###"});
	pg::VoxelGridCellLookup lookup(fixture.grid(), pg::test::BoardFixture::registry());
	pg::BoardOverlayState state;
	pg::BoardOverlayView view;
	view.bind(state, lookup, {0, 0, 0}, flatMasks());

	// First poll after bind rebuilds (view starts at counter 0, state at 1); a second poll is a no-op.
	EXPECT_TRUE(view.rebuildIfNeeded());
	EXPECT_FALSE(view.rebuildIfNeeded());

	// A batch of edits collapses to a single rebuild.
	state.set(OverlayCategory::MoveRange, {{0, 0, 0}, {1, 0, 0}});
	state.set(OverlayCategory::Hovered, {{1, 0, 0}});
	EXPECT_TRUE(view.rebuildIfNeeded());
	EXPECT_FALSE(view.rebuildIfNeeded());

	// The populated categories now have draped geometry; the untouched ones stay empty.
	EXPECT_FALSE(view.mesh(OverlayCategory::MoveRange)->indexes().empty());
	EXPECT_FALSE(view.mesh(OverlayCategory::Hovered)->indexes().empty());
	EXPECT_TRUE(view.mesh(OverlayCategory::Deployment)->indexes().empty());
}

TEST(BoardOverlayView, ClearBindingStopsRebuilding)
{
	pg::test::BoardFixture fixture(std::vector<std::string>{"##", "##"});
	pg::VoxelGridCellLookup lookup(fixture.grid(), pg::test::BoardFixture::registry());
	pg::BoardOverlayState state;
	pg::BoardOverlayView view;
	view.bind(state, lookup, {0, 0, 0}, flatMasks());
	EXPECT_TRUE(view.rebuildIfNeeded());

	view.clearBinding();
	EXPECT_FALSE(view.bound());
	state.set(OverlayCategory::MoveRange, {{0, 0, 0}});
	EXPECT_FALSE(view.rebuildIfNeeded());
}
