#include <gtest/gtest.h>

#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

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

// ---- Per-alignment-combination snapshot tests ----

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

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "PushButtonVisual", "pressed_left_top", captureRect);

	EXPECT_TRUE(result.matches);
}
