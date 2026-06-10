#include <gtest/gtest.h>

#include "structures/widget/spk_checkable_icon_button.hpp"
#include "structures/widget/spk_icon_button.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(IconButtonVisualTest, RendersDefaultIcon)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(IconButtonVisualTest, RendersDifferentIconID)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");
	button.setIconSpriteID(3);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "icon_id_3", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(IconButtonVisualTest, RendersFlat)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::IconButton button("Icon");
	button.setFlat(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "IconButtonVisual", "flat", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CheckableIconButtonVisualTest, RendersUnchecked)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::CheckableIconButton button("Checkable", 0, 1);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "CheckableIconButtonVisual", "unchecked", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CheckableIconButtonVisualTest, RendersChecked)
{
	const spk::Rect2D captureRect(0, 0, 48, 48);

	spk::CheckableIconButton button("Checkable", 0, 1);
	button.setChecked(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "CheckableIconButtonVisual", "checked", captureRect);

	EXPECT_TRUE(result.matches);
}
