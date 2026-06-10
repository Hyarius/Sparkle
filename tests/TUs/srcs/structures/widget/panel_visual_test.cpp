#include <gtest/gtest.h>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(PanelVisualTest, RendersDefaultNineSlice)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);

	spk::Panel panel("Panel");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "PanelVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PanelVisualTest, RendersPressedStyleNineSlice)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);

	spk::Panel panel("Panel", spk::WidgetStyle::makeDefaultPressed());

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "PanelVisual", "pressed_style", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PanelVisualTest, CustomCornerSizeAffectsNineSlice)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);

	spk::Panel panel("Panel");
	panel.setCornerSize({20, 16});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "PanelVisual", "custom_corner_size", captureRect);

	EXPECT_TRUE(result.matches);
}
