#include <gtest/gtest.h>

#include "structures/widget/spk_scroll_bar.hpp"
#include "structures/widget/spk_slider_bar.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "type/spk_orientation.hpp"

TEST(SliderBarVisualTest, RendersHorizontalAtZero)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(0.0f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_zero", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersHorizontalAtMiddle)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersHorizontalAtFull)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setRatio(1.0f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "horizontal_full", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersVerticalAtMiddle)
{
	const spk::Rect2D captureRect(0, 0, 32, 240);

	spk::SliderBar slider("Slider", spk::Orientation::Vertical);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "vertical_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(SliderBarVisualTest, RendersLargeScaleBody)
{
	const spk::Rect2D captureRect(0, 0, 240, 32);

	spk::SliderBar slider("Slider", spk::Orientation::Horizontal);
	slider.setScale(0.5f);
	slider.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(slider, "SliderBarVisual", "large_scale_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ScrollBarVisualTest, RendersHorizontal)
{
	const spk::Rect2D captureRect(0, 0, 240, 24);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Horizontal);
	bar.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "ScrollBarVisual", "horizontal_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ScrollBarVisualTest, RendersVertical)
{
	const spk::Rect2D captureRect(0, 0, 24, 240);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Vertical);
	bar.setRatio(0.5f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "ScrollBarVisual", "vertical_middle", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(ScrollBarVisualTest, RendersSmallScale)
{
	const spk::Rect2D captureRect(0, 0, 240, 24);

	spk::ScrollBar bar("ScrollBar", spk::Orientation::Horizontal);
	bar.setScale(0.2f);
	bar.setRatio(0.3f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "ScrollBarVisual", "horizontal_small_scale", captureRect);

	EXPECT_TRUE(result.matches);
}
