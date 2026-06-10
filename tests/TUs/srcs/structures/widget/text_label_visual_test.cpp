#include <gtest/gtest.h>

#include "structures/widget/spk_text_label.hpp"
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

	spk::WidgetStyle makeAlignmentTestStyle()
	{
		spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
		style.setTextSize(spk::Font::Size(18, 2));
		style.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
		style.setOutlineColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
		style.setTextPadding({0, 0});
		return style;
	}
}

// ---- Per-alignment-combination snapshot tests ----

class TextLabelAlignmentVisualTest : public ::testing::TestWithParam<AlignmentParam>
{
};

TEST_P(TextLabelAlignmentVisualTest, RendersTextAtExpectedPosition)
{
	const auto& param = GetParam();
	const spk::Rect2D captureRect(0, 0, 240, 80);

	spk::TextLabel label("Label", "Hello", makeAlignmentTestStyle());
	label.setAlignment(param.horizontal, param.vertical);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "TextLabelVisual", param.name, captureRect);

	EXPECT_TRUE(result.matches);
}

INSTANTIATE_TEST_SUITE_P(
	AllAlignments,
	TextLabelAlignmentVisualTest,
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

// ---- Padding shifts the anchor point ----

TEST(TextLabelVisualTest, PaddingShiftsTextAnchorInward)
{
	const spk::Rect2D captureRect(0, 0, 240, 80);
	spk::WidgetStyle style = makeAlignmentTestStyle();
	style.setTextPadding({20, 15});

	spk::TextLabel label("Label", "Hello", style);
	label.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "TextLabelVisual", "padded_left_top", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextLabelVisualTest, PaddingShiftsCenteredAnchor)
{
	const spk::Rect2D captureRect(0, 0, 240, 80);
	spk::WidgetStyle style = makeAlignmentTestStyle();
	style.setTextPadding({20, 10});

	spk::TextLabel label("Label", "Hello", style);
	label.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "TextLabelVisual", "padded_centered_centered", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextLabelVisualTest, PaddingShiftsRightDownAnchorInward)
{
	const spk::Rect2D captureRect(0, 0, 240, 80);
	spk::WidgetStyle style = makeAlignmentTestStyle();
	style.setTextPadding({20, 15});

	spk::TextLabel label("Label", "Hello", style);
	label.setAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Down);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "TextLabelVisual", "padded_right_down", captureRect);

	EXPECT_TRUE(result.matches);
}
