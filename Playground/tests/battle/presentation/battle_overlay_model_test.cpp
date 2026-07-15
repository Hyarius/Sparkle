#include <gtest/gtest.h>

#include "battle/presentation/battle_overlay_model.hpp"

#include <vector>

TEST(BattleOverlayModel, ResolvesPriorityAndCanonicalOrderWithoutLayering)
{
	pg::BattleOverlayModelBuilder builder;
	ASSERT_TRUE(builder.update({
		{{2, 0, 1}, pg::BattleMaskKind::Reachable},
		{{2, 0, 1}, pg::BattleMaskKind::AreaPreview},
		{{2, 0, 1}, pg::BattleMaskKind::LineOfSightBlocked},
		{{2, 0, 1}, pg::BattleMaskKind::Hovered},
		{{2, 0, 1}, pg::BattleMaskKind::Invalid},
		{{0, 1, 0}, pg::BattleMaskKind::Path}}));

	const pg::BattleOverlayModel &model = builder.model();
	ASSERT_EQ(model.cells.size(), 2U);
	EXPECT_EQ(model.cells[0].cell, pg::BoardCell(0, 1, 0));
	EXPECT_EQ(model.cells[0].kind, pg::BattleMaskKind::Path);
	EXPECT_EQ(model.cells[1].cell, pg::BoardCell(2, 0, 1));
	EXPECT_EQ(model.cells[1].kind, pg::BattleMaskKind::Invalid);
}

TEST(BattleOverlayModel, KeepsTheRevisionStableForTheSameEffectiveOutput)
{
	pg::BattleOverlayModelBuilder builder;
	ASSERT_TRUE(builder.update({
		{{1, 0, 1}, pg::BattleMaskKind::Reachable},
		{{1, 0, 1}, pg::BattleMaskKind::Path}}));
	const std::uint64_t revision = builder.model().revision;

	EXPECT_FALSE(builder.update({{{1, 0, 1}, pg::BattleMaskKind::Path}}));
	EXPECT_EQ(builder.model().revision, revision);
	EXPECT_TRUE(builder.update({{{1, 0, 1}, pg::BattleMaskKind::Hovered}}));
	EXPECT_EQ(builder.model().revision, revision + 1U);
}

TEST(BattleOverlayModel, UsesTheEightFixedMaskKeys)
{
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::Deployment), "deployment");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::Reachable), "movement");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::Path), "path");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::AbilityRange), "abilityRange");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::AreaPreview), "areaOfEffect");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::LineOfSightBlocked), "losBlocked");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::Hovered), "hovered");
	EXPECT_EQ(pg::overlayMaskKey(pg::BattleMaskKind::Invalid), "invalid");
}
