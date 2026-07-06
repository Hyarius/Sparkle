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

TEST(BattleBanner, ImpressedMessageAutoHidesWithoutConfirmation)
{
	pg::BattleBanner banner("Notice");
	banner.showMessage("Sprout was impressed!", 0.5f);
	EXPECT_TRUE(banner.showing());

	spk::UpdateTick tick{};
	tick.deltaTime = spk::Duration(0.6L, spk::TimeUnit::Second);
	banner.update(tick);

	EXPECT_FALSE(banner.showing());
}

TEST(BattleBanner, ResultWaitsUntilImpressedMessageHasDisplayed)
{
	pg::BattleBanner banner("QueuedResult");
	int confirmations = 0;
	banner.showMessage("Sprout was impressed!", 0.5f);
	banner.showResult(pg::BattleSide::Player, [&confirmations] { ++confirmations; });
	spk::UpdateTick notificationTick{};
	notificationTick.deltaTime = spk::Duration(0.6L, spk::TimeUnit::Second);
	banner.update(notificationTick);
	EXPECT_TRUE(banner.showing());
	EXPECT_EQ(confirmations, 0);

	spk::UpdateTick resultTick{};
	resultTick.deltaTime = spk::Duration(2.1L, spk::TimeUnit::Second);
	banner.update(resultTick);
	EXPECT_FALSE(banner.showing());
	EXPECT_EQ(confirmations, 1);
}
