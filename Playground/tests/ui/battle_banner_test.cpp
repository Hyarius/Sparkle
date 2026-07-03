#include "ui/battle_banner.hpp"

#include "type/spk_time_unit.hpp"

#include <gtest/gtest.h>

TEST(BattleBanner, AutoConfirmsOnceAfterTwoSeconds)
{
	pg::BattleBanner banner("Result");
	int confirmations = 0;
	banner.showResult(pg::BattleSide::Player, [&confirmations] {
		++confirmations;
	});
	EXPECT_TRUE(banner.showing());

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(2.1L, spk::TimeUnit::Second);
	banner.update(tick);
	banner.update(tick);

	EXPECT_EQ(confirmations, 1);
	EXPECT_FALSE(banner.showing());
}
