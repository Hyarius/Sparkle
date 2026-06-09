#include <gtest/gtest.h>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(WidgetStyleVisualTest, PanelRendersNineSliceFromStyle)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);
	const std::string variant = "panel_default_style";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();

	spk::Panel panel("Panel", style);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PanelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);
	const std::string variant = "panel_edited_style";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::Panel panel("Panel", style);

	style.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefaultPressed().nineSliceSpriteSheet());
	style.setNineSliceCornerSize({10, 14});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, TextLabelRendersTextStylingFromStyle)
{
	const spk::Rect2D captureRect(0, 0, 96, 48);
	const std::string variant = "text_label_style";
	const std::string text = "Hi";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setTextSize(spk::Font::Size(18, 3));
	style.setGlyphColor(spk::Color(0.0f, 0.85f, 0.2f, 1.0f));
	style.setOutlineColor(spk::Color(0.95f, 0.1f, 0.1f, 1.0f));
	style.setTextPadding({6, 5});

	spk::TextLabel label("Label", text, style);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, TextLabelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 48);
	const std::string variant = "text_label_edited_style";
	const std::string text = "Hi";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextLabel label("Label", text, style);

	style.setTextSize(spk::Font::Size(20, 1));
	style.setGlyphColor(spk::Color(0.1f, 0.3f, 1.0f, 1.0f));
	style.setOutlineColor(spk::Color(1.0f, 0.9f, 0.0f, 1.0f));
	style.setTextPadding({12, 8});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonRendersReleasedStyleWhenIdle)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_released_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	releasedStyle.setTextSize(spk::Font::Size(16, 2));
	releasedStyle.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
	releasedStyle.setTextPadding({8, 4});
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();

	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonRendersPressedStyleWhilePressed)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_pressed_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	pressedStyle.setTextSize(spk::Font::Size(18, 2));
	pressedStyle.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	pressedStyle.setTextPadding({10, 6});

	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	button.setGeometry(captureRect);

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonPressedVisualRefreshesWhenPressedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_pressed_edited_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	button.setGeometry(captureRect);

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	pressedStyle.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefault().nineSliceSpriteSheet());
	pressedStyle.setNineSliceCornerSize({9, 11});
	pressedStyle.setTextSize(spk::Font::Size(14, 4));
	pressedStyle.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
	pressedStyle.setOutlineColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	pressedStyle.setTextPadding({4, 2});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}
