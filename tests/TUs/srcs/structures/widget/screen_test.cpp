#include <gtest/gtest.h>

#include "structures/widget/spk_screen.hpp"
#include "structures/widget/spk_spacer_widget.hpp"

TEST(SpacerWidgetTest, ConstructedActivatedAndRendersNothing)
{
	spk::SpacerWidget spacer("Spacer");
	spacer.setGeometry(spk::Rect2D(0, 0, 50, 50));

	auto renderUnit = spacer.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
	EXPECT_TRUE(spacer.isActivated());
}

TEST(ScreenTest, ActivationDeactivatesPreviousScreen)
{
	spk::Screen firstScreen("FirstScreen");
	spk::Screen secondScreen("SecondScreen");

	firstScreen.activate();
	EXPECT_TRUE(firstScreen.isActivated());
	EXPECT_EQ(spk::Screen::activeScreen(), &firstScreen);

	secondScreen.activate();
	EXPECT_FALSE(firstScreen.isActivated());
	EXPECT_TRUE(secondScreen.isActivated());
	EXPECT_EQ(spk::Screen::activeScreen(), &secondScreen);
}

TEST(ScreenTest, ReactivatingActiveScreenKeepsItActive)
{
	spk::Screen screen("Screen");

	screen.activate();
	screen.activate();

	EXPECT_TRUE(screen.isActivated());
	EXPECT_EQ(spk::Screen::activeScreen(), &screen);
}

TEST(ScreenTest, DestructionClearsActiveScreen)
{
	{
		spk::Screen screen("Screen");
		screen.activate();
		ASSERT_EQ(spk::Screen::activeScreen(), &screen);
	}

	EXPECT_EQ(spk::Screen::activeScreen(), nullptr);
}
