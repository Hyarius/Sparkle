#include <gtest/gtest.h>

#include "battle/presentation/battle_hud_view_model.hpp"
#include "battle/presentation/battle_unit_position.hpp"

namespace
{
	pg::BattleUnitSnapshot unit(std::uint32_t p_id, pg::BattleSide p_side, std::uint32_t p_order, std::int64_t p_fill)
	{
		pg::BattleUnitSnapshot result;
		result.id = pg::BattleUnitId(p_id);
		result.side = p_side;
		result.rosterOrder = p_order;
		result.placed = true;
		result.cell = pg::BoardCell{static_cast<int>(p_id), 0, 0};
		result.health = 10;
		result.maxHealth = 10;
		result.stamina = pg::BattleTime::fromTicks(100);
		result.turnBarFill = pg::BattleTime::fromTicks(p_fill);
		return result;
	}
}

TEST(BattleHudProjection, ForecastUsesCopiedBarsAndCanonicalTieBreaking)
{
	pg::BattleSnapshot snapshot;
	snapshot.elapsed = pg::BattleTime::fromTicks(1000);
	snapshot.units = {
		unit(2, pg::BattleSide::Enemy, 0, 50),
		unit(1, pg::BattleSide::Player, 0, 20)};
	ASSERT_FALSE(snapshot.units[0].stamina.isZero());
	ASSERT_TRUE(snapshot.units[0].placed);
	ASSERT_TRUE(snapshot.units[0].cell.has_value());
	ASSERT_EQ(snapshot.units[0].health, 10);

	const std::vector<pg::TurnForecastEntry> forecast = pg::forecastActivations(snapshot, 3);
	ASSERT_EQ(forecast.size(), 3U);
	EXPECT_EQ(forecast[0].unit, pg::BattleUnitId(2));
	EXPECT_EQ(forecast[0].readyAt, pg::BattleTime::fromTicks(1050));
	EXPECT_EQ(forecast[1].unit, pg::BattleUnitId(1));
	EXPECT_EQ(forecast[1].readyAt, pg::BattleTime::fromTicks(1080));
	EXPECT_EQ(forecast[2].unit, pg::BattleUnitId(2));
	EXPECT_EQ(forecast[2].readyAt, pg::BattleTime::fromTicks(1150));
	EXPECT_EQ(snapshot.units[0].turnBarFill, pg::BattleTime::fromTicks(50));
}

TEST(BattleUnitVisualConversion, UsesExactChannelsAndStableFallback)
{
	const pg::PlaceholderVisual visual{.tint = {0, 127, 255, 64}, .scalePermille = 250};
	const spk::Color color = pg::toSpkColor(visual);
	EXPECT_FLOAT_EQ(color.r, 0.0f);
	EXPECT_FLOAT_EQ(color.g, 127.0f / 255.0f);
	EXPECT_FLOAT_EQ(color.b, 1.0f);
	EXPECT_FLOAT_EQ(color.a, 64.0f / 255.0f);
	EXPECT_FLOAT_EQ(pg::toWorldScale(visual), 0.25f);
	EXPECT_FLOAT_EQ(pg::toWorldScale({.scalePermille = 3000}), 3.0f);
	EXPECT_EQ(pg::fallbackPlaceholderVisual("species/base"), pg::fallbackPlaceholderVisual("species/base"));
	EXPECT_NE(pg::fallbackPlaceholderVisual("species/base"), pg::fallbackPlaceholderVisual("other/base"));
}
