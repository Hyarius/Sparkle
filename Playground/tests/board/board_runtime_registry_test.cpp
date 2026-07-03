#include "board/board_runtime_registry.hpp"

#include <gtest/gtest.h>

TEST(BoardRuntimeRegistry, EnforcesSingleUnitOccupancyAndSupportsMoveSwapRemove)
{
	pg::BoardRuntimeRegistry registry;
	pg::BattleObject first;
	pg::BattleObject second;
	const spk::Vector3Int left{0, 0, 0};
	const spk::Vector3Int right{1, 0, 0};

	EXPECT_TRUE(registry.tryRegister(first, left));
	EXPECT_FALSE(registry.tryRegister(first, right));
	EXPECT_FALSE(registry.tryRegister(second, left));
	EXPECT_TRUE(registry.tryRegister(second, right));
	EXPECT_FALSE(registry.tryMove(first, right));
	EXPECT_TRUE(registry.swapUnits(first, second));
	EXPECT_EQ(registry.tryGetPosition(first), right);
	EXPECT_EQ(registry.tryGetPosition(second), left);
	EXPECT_EQ(registry.tryGetUnitAt(right), &first);
	EXPECT_TRUE(registry.remove(first));
	EXPECT_FALSE(registry.hasUnitAt(right));
	EXPECT_FALSE(registry.remove(first));
}

TEST(BoardRuntimeRegistry, StacksObjectsRemovesByTagsAndAdvancesDurations)
{
	pg::BoardRuntimeRegistry registry;
	pg::BattleObject fire;
	pg::BattleObject ice;
	pg::BattleObject marker;
	fire.tags = {"hazard", "fire"};
	ice.tags = {"hazard", "ice"};
	marker.tags = {"marker"};
	const spk::Vector3Int cell{2, 0, 3};

	EXPECT_TRUE(registry.tryAdd(fire, cell, 1));
	EXPECT_TRUE(registry.tryAdd(ice, cell, 2));
	EXPECT_TRUE(registry.tryAdd(marker, cell));
	EXPECT_FALSE(registry.tryAdd(marker, cell));
	EXPECT_EQ(registry.getAt(cell).size(), 3);

	const std::vector<pg::BattleObject *> firstExpired = registry.advanceObjectDurations();
	EXPECT_EQ(firstExpired, (std::vector<pg::BattleObject *>{&fire}));
	EXPECT_EQ(registry.getAt(cell).size(), 2);
	EXPECT_EQ(registry.removeByTags(cell, {"hazard", "ice"}), 1);
	EXPECT_EQ(registry.getAt(cell), (std::vector<pg::BattleObject *>{&marker}));
	EXPECT_TRUE(registry.advanceObjectDurations().empty());
}
