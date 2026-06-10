#include <gtest/gtest.h>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"

TEST(WidgetStyleReactivityTest, PanelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::Panel panel("Panel", style);

	style.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefaultPressed().nineSliceSpriteSheet());
	style.setNineSliceCornerSize({10, 14});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "WidgetStyleReactivity", "panel_edited_style", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(WidgetStyleReactivityTest, TextLabelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 48);
	const std::string text = "Hi";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextLabel label("Label", text, style);

	style.setTextSize(spk::Font::Size(20, 1));
	style.setGlyphColor(spk::Color(0.1f, 0.3f, 1.0f, 1.0f));
	style.setOutlineColor(spk::Color(1.0f, 0.9f, 0.0f, 1.0f));
	style.setTextPadding({12, 8});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "WidgetStyleReactivity", "text_label_edited_style", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(WidgetStyleReactivityTest, PushButtonPressedVisualRefreshesWhenPressedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	button.setGeometry(captureRect);

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {10, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	pressedStyle.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefault().nineSliceSpriteSheet());
	pressedStyle.setNineSliceCornerSize({9, 11});
	pressedStyle.setTextSize(spk::Font::Size(14, 4));
	pressedStyle.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
	pressedStyle.setOutlineColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	pressedStyle.setTextPadding({4, 2});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleReactivity", "push_button_pressed_edited_style", captureRect);

	EXPECT_TRUE(result.matches);
}
