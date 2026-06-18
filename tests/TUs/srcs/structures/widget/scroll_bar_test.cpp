#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_scroll_bar.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(ScrollBarTest, DefaultStateIsHorizontal)
{
	spk::ScrollBar scrollBar("ScrollBar");

	EXPECT_EQ(scrollBar.orientation(), spk::Orientation::Horizontal);
	EXPECT_FLOAT_EQ(scrollBar.ratio(), 0.0f);
	EXPECT_FLOAT_EQ(scrollBar.step(), 0.1f);
	EXPECT_TRUE(scrollBar.isActivated());
}

TEST(ScrollBarTest, HorizontalGeometryPlacesButtonsOnEachSide)
{
	spk::ScrollBar scrollBar("ScrollBar");
	scrollBar.setGeometry(spk::Rect2D(0, 0, 120, 20));

	EXPECT_EQ(scrollBar.negativeButton().geometry(), spk::Rect2D(0, 0, 20, 20));
	EXPECT_EQ(scrollBar.sliderBar().geometry(), spk::Rect2D(20, 0, 80, 20));
	EXPECT_EQ(scrollBar.positiveButton().geometry(), spk::Rect2D(100, 0, 20, 20));
}

TEST(ScrollBarTest, VerticalGeometryStacksButtonsVertically)
{
	spk::ScrollBar scrollBar("ScrollBar", spk::Orientation::Vertical);
	scrollBar.setGeometry(spk::Rect2D(0, 0, 20, 120));

	EXPECT_EQ(scrollBar.negativeButton().geometry(), spk::Rect2D(0, 0, 20, 20));
	EXPECT_EQ(scrollBar.sliderBar().geometry(), spk::Rect2D(0, 20, 20, 80));
	EXPECT_EQ(scrollBar.positiveButton().geometry(), spk::Rect2D(0, 100, 20, 20));
}

TEST(ScrollBarTest, PositiveButtonClickIncreasesRatioByStep)
{
	spk::ScrollBar scrollBar("ScrollBar");
	scrollBar.setGeometry(spk::Rect2D(0, 0, 120, 20));

	float lastRatio = -1.0f;
	auto contract = scrollBar.subscribeToEdition([&lastRatio](float p_ratio) { lastRatio = p_ratio; });

	spk::MouseModule mouseModule;
	mouseModule.bind(&scrollBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {110, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FLOAT_EQ(scrollBar.ratio(), 0.1f);
	EXPECT_FLOAT_EQ(lastRatio, 0.1f);
}

TEST(ScrollBarTest, NegativeButtonClickDecreasesRatioByStep)
{
	spk::ScrollBar scrollBar("ScrollBar");
	scrollBar.setGeometry(spk::Rect2D(0, 0, 120, 20));
	scrollBar.setRatio(0.5f);

	spk::MouseModule mouseModule;
	mouseModule.bind(&scrollBar);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {10, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FLOAT_EQ(scrollBar.ratio(), 0.4f);
}

TEST(ScrollBarTest, SetRatioClampsToValidRange)
{
	spk::ScrollBar scrollBar("ScrollBar");

	scrollBar.setRatio(2.0f);
	EXPECT_FLOAT_EQ(scrollBar.ratio(), 1.0f);

	scrollBar.setRatio(-1.0f);
	EXPECT_FLOAT_EQ(scrollBar.ratio(), 0.0f);
}

TEST(ScrollBarTest, SetStepRejectsOutOfRangeValues)
{
	spk::ScrollBar scrollBar("ScrollBar");

	EXPECT_THROW(scrollBar.setStep(0.0f), std::invalid_argument);
	EXPECT_THROW(scrollBar.setStep(1.5f), std::invalid_argument);
}

TEST(ScrollBarTest, SetOrientationUpdatesButtonIcons)
{
	spk::ScrollBar scrollBar("ScrollBar");

	EXPECT_TRUE(scrollBar.negativeButton().hasIcon());
	EXPECT_TRUE(scrollBar.positiveButton().hasIcon());

	const auto horizontalNegativeSection = scrollBar.negativeButton().releasedIcon().section();

	scrollBar.setOrientation(spk::Orientation::Vertical);

	EXPECT_EQ(scrollBar.orientation(), spk::Orientation::Vertical);
	EXPECT_NE(scrollBar.negativeButton().releasedIcon().section(), horizontalNegativeSection);
}

TEST(ScrollBarVisualTest, RendersHorizontal)
{
	const spk::Rect2D captureRect(0, 0, 240, 24);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Horizontal);
	bar.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(bar, "ScrollBarVisual", "horizontal_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ScrollBarVisualTest, RendersVertical)
{
	const spk::Rect2D captureRect(0, 0, 24, 240);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Vertical);
	bar.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(bar, "ScrollBarVisual", "vertical_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ScrollBarVisualTest, RendersSmallScale)
{
	const spk::Rect2D captureRect(0, 0, 240, 24);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Horizontal);
	bar.setScale(0.2f);
	bar.setRatio(0.3f);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(bar, "ScrollBarVisual", "horizontal_small_scale", captureRect);

	EXPECT_TRUE(result.matches);
}
