#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

#include "spk_generated_resources.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> makeInvalidGridSpriteSheet()
	{
		const auto& bytes = SPARKLE_GET_RESOURCE("resources/textures/default_nine_slice.png");
		return std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
				spk::Vector2UInt{2, 2}));
	}
}

TEST(PanelTest, BuildsNineSliceRenderCommandFromDefaultStyle)
{
	spk::Panel panel("Panel");
	panel.setGeometry(spk::Rect2D(0, 0, 96, 48));

	std::shared_ptr<spk::RenderUnit> renderUnit = panel.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
	EXPECT_TRUE(panel.isActivated());
}

TEST(PanelTest, ConstructedWithSpriteSheetIsActivated)
{
	auto sheet = spk::WidgetStyle::makeDefault().nineSliceSpriteSheet();
	spk::Panel panel("Panel", sheet);

	EXPECT_EQ(panel.spriteSheet(), sheet);
	EXPECT_TRUE(panel.isActivated());
}

TEST(PanelTest, SpritesheetAndDepthGetters)
{
	spk::Panel panel("Panel");

	EXPECT_NE(panel.spriteSheet(), nullptr);
	EXPECT_FLOAT_EQ(panel.depth(), 0.0f);
}

TEST(PanelTest, SetDepthUpdatesValue)
{
	spk::Panel panel("Panel");

	panel.setDepth(5.0f);

	EXPECT_FLOAT_EQ(panel.depth(), 5.0f);
}

TEST(PanelTest, SetDepthIgnoresSameValue)
{
	spk::Panel panel("Panel");
	panel.setDepth(2.0f);

	panel.setDepth(2.0f);

	EXPECT_FLOAT_EQ(panel.depth(), 2.0f);
}

TEST(PanelTest, SetSpriteSheetRejectsNull)
{
	spk::Panel panel("Panel");

	EXPECT_THROW(panel.setSpriteSheet(nullptr), std::invalid_argument);
}

TEST(PanelTest, SetSpriteSheetRejectsNonThreeByThreeGrid)
{
	spk::Panel panel("Panel");

	EXPECT_THROW(panel.setSpriteSheet(makeInvalidGridSpriteSheet()), std::invalid_argument);
}

TEST(PanelTest, SetCornerSizeRejectsNegativeComponents)
{
	spk::Panel panel("Panel");

	EXPECT_THROW(panel.setCornerSize({-1, 0}), std::invalid_argument);
	EXPECT_THROW(panel.setCornerSize({0, -1}), std::invalid_argument);
}

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
