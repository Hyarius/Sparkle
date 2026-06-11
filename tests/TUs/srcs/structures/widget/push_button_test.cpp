#include <gtest/gtest.h>

#include <string_view>

#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

TEST(PushButtonTest, BuildsSkinAndTextRenderCommands)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));

	auto releasedBgUnit = button.releasedBackground().renderUnit();
	auto releasedLabelUnit = button.releasedLabel().renderUnit();

	ASSERT_NE(releasedBgUnit, nullptr);
	ASSERT_NE(releasedLabelUnit, nullptr);
	EXPECT_EQ(releasedBgUnit->size(), 1u);
	EXPECT_EQ(releasedLabelUnit->size(), 1u);
	EXPECT_TRUE(button.isActivated());
}

TEST(PushButtonTest, TriggersClickOnLeftPressAndReleaseInside)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	int clickCount = 0;
	auto contract = button.subscribeToClick([&clickCount]()
	{
		++clickCount;
	});

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(button.isPressed());

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(button.isPressed());
	EXPECT_EQ(clickCount, 1);
}

TEST(PushButtonTest, UsesReleasedAndPressedStyles)
{
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	releasedStyle.setTextSize(spk::Font::Size(16, 0));
	pressedStyle.setTextSize(spk::Font::Size(24, 1));

	spk::PushButton button("Button", "OK", releasedStyle, pressedStyle);
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	EXPECT_EQ(button.releasedLabel().textSize(), spk::Font::Size(16, 0));
	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(24, 1));

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(button.isPressed());
	EXPECT_TRUE(button.pressedLabel().isActivated());
	EXPECT_FALSE(button.releasedLabel().isActivated());

	pressedStyle.setTextSize(spk::Font::Size(28, 2));

	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(28, 2));
}

TEST(PushButtonTest, NameOnlyConstructorActivates)
{
	spk::PushButton button("Button");

	EXPECT_TRUE(button.isActivated());
	EXPECT_TRUE(button.releasedLabel().text().empty());
	EXPECT_TRUE(button.pressedLabel().text().empty());
}

TEST(PushButtonTest, SingleStyleConstructorAppliesStyle)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setTextSize(spk::Font::Size(20, 1));

	spk::PushButton button("Button", "OK", style);

	EXPECT_EQ(button.releasedLabel().textSize(), spk::Font::Size(20, 1));
	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(20, 1));
	EXPECT_TRUE(button.isActivated());
}

TEST(PushButtonTest, ReleasedBackgroundUpdatesWhenStyleChanges)
{
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	const auto newSheet = spk::WidgetStyle::makeDefaultPressed().nineSliceSpriteSheet();
	releasedStyle.setNineSliceSpriteSheet(newSheet);

	spk::PushButton button("Button", "OK", releasedStyle, spk::WidgetStyle::makeDefaultPressed());

	EXPECT_EQ(button.releasedBackground().spriteSheet(), newSheet);
}

TEST(PushButtonTest, PressedBackgroundIsActiveWhenPressed)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	EXPECT_TRUE(button.releasedBackground().isActivated());
	EXPECT_FALSE(button.pressedBackground().isActivated());

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(button.releasedBackground().isActivated());
	EXPECT_TRUE(button.pressedBackground().isActivated());
}

TEST(PushButtonTest, SetFontNullThrows)
{
	spk::PushButton button("Button", "OK");

	EXPECT_THROW(button.releasedLabel().setFont(nullptr), std::invalid_argument);
	EXPECT_THROW(button.pressedLabel().setFont(nullptr), std::invalid_argument);
}

TEST(PushButtonTest, SetFontUpdatesSuccessfully)
{
	spk::PushButton button("Button", "OK");
	auto font = spk::WidgetStyle::makeDefault().font();

	button.releasedLabel().setFont(font);
	button.pressedLabel().setFont(font);

	EXPECT_EQ(button.releasedLabel().font(), font);
	EXPECT_EQ(button.pressedLabel().font(), font);
}

TEST(PushButtonTest, SetTextSameIsNoOp)
{
	spk::PushButton button("Button", "Hello");
	const auto currentText = button.releasedLabel().text();

	button.setText(currentText);

	EXPECT_EQ(button.releasedLabel().text(), currentText);
	EXPECT_EQ(button.pressedLabel().text(), currentText);
}

TEST(PushButtonTest, SetTextDifferentUpdates)
{
	spk::PushButton button("Button", "Hello");
	const auto newText = spk::Font::textFromUTF8("World");

	button.setText(newText);

	EXPECT_EQ(button.releasedLabel().text(), newText);
	EXPECT_EQ(button.pressedLabel().text(), newText);
}

TEST(PushButtonTest, SetTextStringViewUpdates)
{
	spk::PushButton button("Button", "Hello");

	button.setText(std::string_view("Updated"));

	EXPECT_EQ(button.releasedLabel().text(), spk::Font::textFromUTF8("Updated"));
	EXPECT_EQ(button.pressedLabel().text(), spk::Font::textFromUTF8("Updated"));
}

TEST(PushButtonTest, SetTextSizeSameIsNoOp)
{
	spk::PushButton button("Button", "OK");
	const auto currentSize = button.releasedLabel().textSize();

	button.releasedLabel().setTextSize(currentSize);

	EXPECT_EQ(button.releasedLabel().textSize(), currentSize);
}

TEST(PushButtonTest, SetGlyphColorSameIsNoOp)
{
	spk::PushButton button("Button", "OK");
	const auto currentColor = button.releasedLabel().glyphColor();

	button.releasedLabel().setGlyphColor(currentColor);

	EXPECT_EQ(button.releasedLabel().glyphColor().r, currentColor.r);
	EXPECT_EQ(button.releasedLabel().glyphColor().g, currentColor.g);
}

TEST(PushButtonTest, SetOutlineColorSameIsNoOp)
{
	spk::PushButton button("Button", "OK");
	const auto currentColor = button.releasedLabel().outlineColor();

	button.releasedLabel().setOutlineColor(currentColor);

	EXPECT_EQ(button.releasedLabel().outlineColor().r, currentColor.r);
	EXPECT_EQ(button.releasedLabel().outlineColor().g, currentColor.g);
}

TEST(PushButtonTest, SetPaddingSameIsNoOp)
{
	spk::PushButton button("Button", "OK");
	const auto currentPadding = button.releasedLabel().padding();

	button.releasedLabel().setPadding(currentPadding);

	EXPECT_EQ(button.releasedLabel().padding(), currentPadding);
}

TEST(PushButtonTest, SetPaddingDifferentUpdates)
{
	spk::PushButton button("Button", "OK");
	const spk::Vector2Int newPadding = {12, 8};

	button.releasedLabel().setPadding(newPadding);

	EXPECT_EQ(button.releasedLabel().padding(), newPadding);
}

TEST(PushButtonTest, SetDepthSameIsNoOp)
{
	spk::PushButton button("Button", "OK");

	button.releasedLabel().setDepth(0.0f);

	EXPECT_FLOAT_EQ(button.releasedLabel().depth(), 0.0f);
}

TEST(PushButtonTest, SetDepthUpdates)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));

	button.releasedLabel().setDepth(2.5f);

	EXPECT_FLOAT_EQ(button.releasedLabel().depth(), 2.5f);
	EXPECT_NE(button.releasedLabel().renderUnit(), nullptr);
}

TEST(PushButtonTest, GettersReturnCorrectValues)
{
	spk::PushButton button("Button", "Hello");

	EXPECT_FALSE(button.isHovered());
	EXPECT_FALSE(button.isPressed());
	EXPECT_EQ(button.releasedLabel().text(), spk::Font::textFromUTF8("Hello"));
	EXPECT_EQ(button.pressedLabel().text(), spk::Font::textFromUTF8("Hello"));
	EXPECT_EQ(button.releasedLabel().padding(), spk::Vector2Int(0, 0));
}

TEST(PushButtonTest, MouseMovedEventUpdatesHoverState)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	EXPECT_TRUE(button.isHovered());

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 200}})));
	EXPECT_FALSE(button.isHovered());
}

TEST(PushButtonTest, MouseLeftEventEarlyReturnWhenNeitherHoveredNorPressed)
{
	spk::PushButton button("Button", "OK");

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseLeftRecord{})));

	EXPECT_FALSE(button.isHovered());
	EXPECT_FALSE(button.isPressed());
}

TEST(PushButtonTest, MouseLeftEventResetsStateWhenHovered)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	ASSERT_TRUE(button.isHovered());

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 20}})));

	EXPECT_FALSE(button.isHovered());
	EXPECT_FALSE(button.isPressed());
}

TEST(PushButtonTest, MousePressedNonLeftButtonIsIgnored)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Right})));
	mouseModule.processEvents();

	EXPECT_FALSE(button.isPressed());
}

TEST(PushButtonTest, MousePressedOutsideGeometryIsIgnored)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 200}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(button.isPressed());
}

TEST(PushButtonTest, MouseReleasedWhenNotPressedIsIgnored)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	int clickCount = 0;
	auto contract = button.subscribeToClick([&clickCount]() { ++clickCount; });

	sparkle_test::sendMouseEvent(button, spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));

	EXPECT_EQ(clickCount, 0);
}

TEST(PushButtonTest, MouseReleasedOutsideBoundsDoesNotTriggerClick)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	int clickCount = 0;
	auto contract = button.subscribeToClick([&clickCount]() { ++clickCount; });

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();
	ASSERT_TRUE(button.isPressed());

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {200, 200}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(clickCount, 0);
	EXPECT_FALSE(button.isPressed());
}

TEST(PushButtonTest, ReleasedStyleSubscriptionRefreshesWhenNotPressed)
{
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	releasedStyle.setTextSize(spk::Font::Size(12, 0));
	pressedStyle.setTextSize(spk::Font::Size(18, 1));

	spk::PushButton button("Button", "OK", releasedStyle, pressedStyle);

	releasedStyle.setTextSize(spk::Font::Size(14, 0));

	EXPECT_EQ(button.releasedLabel().textSize(), spk::Font::Size(14, 0));
}

TEST(PushButtonTest, PressedStyleSubscriptionRefreshesWhenPressed)
{
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	releasedStyle.setTextSize(spk::Font::Size(12, 0));
	pressedStyle.setTextSize(spk::Font::Size(18, 1));

	spk::PushButton button("Button", "OK", releasedStyle, pressedStyle);
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();
	ASSERT_TRUE(button.isPressed());

	pressedStyle.setTextSize(spk::Font::Size(22, 2));

	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(22, 2));
}

TEST(PushButtonTest, SetFlatDeactivatesBackgrounds)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));

	ASSERT_FALSE(button.isFlat());
	ASSERT_TRUE(button.releasedBackground().isActivated());

	button.setFlat(true);

	EXPECT_TRUE(button.isFlat());
	EXPECT_FALSE(button.releasedBackground().isActivated());
	EXPECT_FALSE(button.pressedBackground().isActivated());
}

TEST(PushButtonTest, SetFlatFalseRestoresReleasedBackground)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));
	button.setFlat(true);

	button.setFlat(false);

	EXPECT_FALSE(button.isFlat());
	EXPECT_TRUE(button.releasedBackground().isActivated());
}

TEST(PushButtonTest, SetFlatNoOpWhenSameState)
{
	spk::PushButton button("Button", "OK");

	button.setFlat(false);
	EXPECT_FALSE(button.isFlat());

	button.setFlat(true);
	const bool prevActivated = button.releasedBackground().isActivated();
	button.setFlat(true);
	EXPECT_EQ(button.releasedBackground().isActivated(), prevActivated);
}

TEST(PushButtonTest, SetAlignmentForwardsToBothLabels)
{
	spk::PushButton button("Button", "OK");

	button.setAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Down);

	EXPECT_EQ(button.releasedLabel().horizontalAlignment(), spk::HorizontalAlignment::Right);
	EXPECT_EQ(button.releasedLabel().verticalAlignment(), spk::VerticalAlignment::Down);
	EXPECT_EQ(button.pressedLabel().horizontalAlignment(), spk::HorizontalAlignment::Right);
	EXPECT_EQ(button.pressedLabel().verticalAlignment(), spk::VerticalAlignment::Down);
}

TEST(PushButtonTest, ApplyStyleSingleUpdatesAllComponents)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setTextSize(spk::Font::Size(18, 1));

	spk::PushButton button("Button", "OK");
	button.applyStyle(style);

	EXPECT_EQ(button.releasedLabel().textSize(), spk::Font::Size(18, 1));
	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(18, 1));
}

TEST(PushButtonTest, UseDefaultStylesRestoresDefault)
{
	spk::WidgetStyle custom = spk::WidgetStyle::makeDefault();
	custom.setTextSize(spk::Font::Size(99, 0));

	spk::PushButton button("Button", "OK", custom, custom);
	button.useDefaultStyles();

	const spk::Font::Size defaultSize =
		spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).textSize();
	EXPECT_EQ(button.releasedLabel().textSize(), defaultSize);
}

TEST(PushButtonTest, SetIconWithSpriteSetsHasIcon)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));
	auto iconset = spk::WidgetStyle::makeDefault().iconSpriteSheet();

	ASSERT_FALSE(button.hasIcon());

	button.setIcon(iconset, 0u);

	EXPECT_TRUE(button.hasIcon());
	EXPECT_TRUE(button.releasedIcon().isActivated());
	EXPECT_FALSE(button.pressedIcon().isActivated());
}

TEST(PushButtonTest, SetIconNullSpriteSheetThrows)
{
	spk::PushButton button("Button", "OK");

	EXPECT_THROW(button.setIcon(nullptr, 0u), std::invalid_argument);
}

TEST(PushButtonTest, RemoveIconClearsState)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(0, 0, 120, 40));
	auto iconset = spk::WidgetStyle::makeDefault().iconSpriteSheet();
	button.setIcon(iconset, 0u);
	ASSERT_TRUE(button.hasIcon());

	button.removeIcon();

	EXPECT_FALSE(button.hasIcon());
	EXPECT_FALSE(button.releasedIcon().isActivated());
	EXPECT_FALSE(button.pressedIcon().isActivated());
}

TEST(PushButtonTest, IconVisibleOnPressedState)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));
	auto iconset = spk::WidgetStyle::makeDefault().iconSpriteSheet();
	button.setIcon(iconset, 0u);

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(button.pressedIcon().isActivated());
	EXPECT_FALSE(button.releasedIcon().isActivated());
}

TEST(PushButtonTest, ConstAccessorsReturnSameObjects)
{
	spk::PushButton button("Button", "OK");
	const spk::PushButton& cbutton = button;

	EXPECT_EQ(&cbutton.releasedBackground(), &button.releasedBackground());
	EXPECT_EQ(&cbutton.pressedBackground(), &button.pressedBackground());
	EXPECT_EQ(&cbutton.releasedLabel(), &button.releasedLabel());
	EXPECT_EQ(&cbutton.pressedLabel(), &button.pressedLabel());
	EXPECT_EQ(&cbutton.releasedIcon(), &button.releasedIcon());
	EXPECT_EQ(&cbutton.pressedIcon(), &button.pressedIcon());
}

TEST(PushButtonTest, FlatPressedStateShowsOnlyLabel)
{
	spk::PushButton button("Button", "OK");
	button.setGeometry(spk::Rect2D(10, 10, 120, 40));
	button.setFlat(true);

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {20, 20}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_TRUE(button.isPressed());
	EXPECT_FALSE(button.pressedBackground().isActivated());
	EXPECT_TRUE(button.pressedLabel().isActivated());
	EXPECT_FALSE(button.releasedBackground().isActivated());
}

// ---- Per-alignment-combination snapshot tests ----

namespace
{
	struct AlignmentParam
	{
		spk::HorizontalAlignment horizontal;
		spk::VerticalAlignment vertical;
		std::string name;
	};

	void configureStyle(spk::WidgetStyle& p_style)
	{
		p_style.setTextSize(spk::Font::Size(16, 2));
		p_style.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
		p_style.setOutlineColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
		p_style.setTextPadding({0, 0});
	}
}

class PushButtonAlignmentVisualTest : public ::testing::TestWithParam<AlignmentParam>
{
};

TEST_P(PushButtonAlignmentVisualTest, RendersTextAtExpectedPosition)
{
	const auto& param = GetParam();
	const spk::Rect2D captureRect(0, 0, 200, 80);

	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	configureStyle(releasedStyle);
	configureStyle(pressedStyle);

	spk::PushButton button("Button", "Hello", releasedStyle, pressedStyle);
	button.setAlignment(param.horizontal, param.vertical);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", param.name, captureRect);

	EXPECT_TRUE(result.matches);
}

INSTANTIATE_TEST_SUITE_P(
	AllAlignments,
	PushButtonAlignmentVisualTest,
	::testing::Values(
		AlignmentParam{spk::HorizontalAlignment::Left,     spk::VerticalAlignment::Top,      "left_top"},
		AlignmentParam{spk::HorizontalAlignment::Left,     spk::VerticalAlignment::Centered,  "left_centered"},
		AlignmentParam{spk::HorizontalAlignment::Left,     spk::VerticalAlignment::Down,      "left_down"},
		AlignmentParam{spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Top,       "centered_top"},
		AlignmentParam{spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered,  "centered_centered"},
		AlignmentParam{spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Down,      "centered_down"},
		AlignmentParam{spk::HorizontalAlignment::Right,    spk::VerticalAlignment::Top,       "right_top"},
		AlignmentParam{spk::HorizontalAlignment::Right,    spk::VerticalAlignment::Centered,  "right_centered"},
		AlignmentParam{spk::HorizontalAlignment::Right,    spk::VerticalAlignment::Down,      "right_down"}
	),
	[](const ::testing::TestParamInfo<AlignmentParam>& info) { return info.param.name; }
);

// ---- Padding shifts the text anchor ----

TEST(PushButtonVisualTest, PaddingShiftsTextAnchorFromTopLeft)
{
	const spk::Rect2D captureRect(0, 0, 200, 80);

	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	configureStyle(releasedStyle);
	configureStyle(pressedStyle);
	releasedStyle.setTextPadding({20, 15});
	pressedStyle.setTextPadding({20, 15});

	spk::PushButton button("Button", "Hello", releasedStyle, pressedStyle);
	button.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", "padded_left_top", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PushButtonVisualTest, PaddingIgnoredForCenteredAlignment)
{
	const spk::Rect2D captureRect(0, 0, 200, 80);

	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	configureStyle(releasedStyle);
	configureStyle(pressedStyle);
	releasedStyle.setTextPadding({20, 10});
	pressedStyle.setTextPadding({20, 10});

	spk::PushButton button("Button", "Hello", releasedStyle, pressedStyle);
	button.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", "padded_centered_centered", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PushButtonVisualTest, PaddingShiftsTextAnchorFromBottomRight)
{
	const spk::Rect2D captureRect(0, 0, 200, 80);

	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	configureStyle(releasedStyle);
	configureStyle(pressedStyle);
	releasedStyle.setTextPadding({20, 15});
	pressedStyle.setTextPadding({20, 15});

	spk::PushButton button("Button", "Hello", releasedStyle, pressedStyle);
	button.setAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Down);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", "padded_right_down", captureRect);

	EXPECT_TRUE(result.matches);
}

// ---- Alignment is preserved across pressed/released state changes ----

TEST(PushButtonVisualTest, AlignmentPreservedWhenPressed)
{
	const spk::Rect2D captureRect(0, 0, 200, 80);

	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	configureStyle(releasedStyle);
	configureStyle(pressedStyle);

	spk::PushButton button("Button", "Hello", releasedStyle, pressedStyle);
	button.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);
	button.setGeometry(captureRect);

	spk::MouseModule mouseModule;
	mouseModule.bind(&button);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = {10, 10}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", "pressed_left_top", captureRect);

	EXPECT_TRUE(result.matches);
}
