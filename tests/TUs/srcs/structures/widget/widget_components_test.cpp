#include <gtest/gtest.h>

#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"

TEST(WidgetComponentsTest, PanelBuildsNineSliceRenderCommandFromDefaultStyle)
{
	spk::Panel panel("Panel");
	panel.setGeometry(spk::Rect2D(0, 0, 96, 48));

	std::shared_ptr<spk::RenderUnit> renderUnit = panel.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
	EXPECT_TRUE(panel.isActivated());
}

TEST(WidgetComponentsTest, TextLabelBuildsTextRenderCommandWhenTextIsNotEmpty)
{
	spk::TextLabel label("Label", "Hello");
	label.setGeometry(spk::Rect2D(0, 0, 120, 40));
	label.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	std::shared_ptr<spk::RenderUnit> renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
	EXPECT_TRUE(label.isActivated());
}

TEST(WidgetComponentsTest, PushButtonBuildsSkinAndTextRenderCommands)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));

	std::shared_ptr<spk::RenderUnit> renderUnit = button.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 2u);
	EXPECT_TRUE(button.isActivated());
}

TEST(WidgetComponentsTest, PushButtonTriggersClickOnLeftPressAndReleaseInside)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	int clickCount = 0;
	auto contract = button.subscribeToClick([&clickCount]()
	{
		++clickCount;
	});

	spk::Mouse mouse;
	mouse.position = {20, 20};

	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	EXPECT_TRUE(button.isPressed());

	spk::MouseEventRecord releaseEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(releaseEvent, mouse);

	EXPECT_FALSE(button.isPressed());
	EXPECT_EQ(clickCount, 1);
}

TEST(WidgetComponentsTest, ExplicitStyleIsAppliedOnConstruction)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setNineSliceCornerSize({4, 5});
	style.setTextSize(spk::Font::Size(24, 2));

	spk::Panel panel("Panel", style);
	spk::TextLabel label("Label", "Hello", style);

	EXPECT_EQ(panel.cornerSize(), spk::Vector2Int(4, 5));
	EXPECT_EQ(label.textSize(), spk::Font::Size(24, 2));
}

TEST(WidgetComponentsTest, WidgetFollowingDefaultStyleRefreshesWhenDefaultStyleIsReplaced)
{
	spk::WidgetStyle originalStyle = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default);
	spk::WidgetStyle firstStyle = originalStyle;
	spk::WidgetStyle secondStyle = originalStyle;

	firstStyle.setTextSize(spk::Font::Size(18, 1));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, firstStyle);
	spk::TextLabel label("Label", "Hello");
	ASSERT_EQ(label.textSize(), spk::Font::Size(18, 1));

	secondStyle.setTextSize(spk::Font::Size(30, 3));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, secondStyle);

	EXPECT_EQ(label.textSize(), spk::Font::Size(30, 3));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, originalStyle);
}

TEST(WidgetComponentsTest, WidgetFollowingDefaultStyleRefreshesWhenDefaultStyleIsEdited)
{
	spk::WidgetStyle originalStyle = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default);
	spk::TextLabel label("Label", "Hello");

	spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).setTextSize(spk::Font::Size(28, 2));

	EXPECT_EQ(label.textSize(), spk::Font::Size(28, 2));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, originalStyle);
}

TEST(WidgetComponentsTest, StyleEditionCallbackReceivesEditedStyle)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::Font::Size receivedSize;

	auto contract = style.subscribeToEdition([&receivedSize](const spk::WidgetStyle& p_style)
	{
		receivedSize = p_style.textSize();
	});

	style.setTextSize(spk::Font::Size(20, 1));

	EXPECT_EQ(receivedSize, spk::Font::Size(20, 1));
}

TEST(WidgetComponentsTest, CollectionCreatesAndReturnsNamedStyles)
{
	spk::WidgetStyle& style = spk::WidgetStyle::Collection::style("warning");
	style.setTextSize(spk::Font::Size(22, 1));

	spk::TextLabel label("Label", "Careful", style);

	EXPECT_TRUE(spk::WidgetStyle::Collection::contains("warning"));
	EXPECT_EQ(label.textSize(), spk::Font::Size(22, 1));
}

TEST(WidgetComponentsTest, PushButtonUsesReleasedAndPressedStyles)
{
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	releasedStyle.setTextSize(spk::Font::Size(16, 0));
	pressedStyle.setTextSize(spk::Font::Size(24, 1));

	spk::PushButton button("Button", "OK", releasedStyle, pressedStyle);
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	EXPECT_EQ(button.textSize(), spk::Font::Size(16, 0));

	spk::Mouse mouse;
	mouse.position = {20, 20};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	EXPECT_EQ(button.textSize(), spk::Font::Size(24, 1));

	pressedStyle.setTextSize(spk::Font::Size(28, 2));

	EXPECT_EQ(button.textSize(), spk::Font::Size(28, 2));
}
