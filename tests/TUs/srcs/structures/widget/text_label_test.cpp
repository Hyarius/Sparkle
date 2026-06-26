#include <gtest/gtest.h>

#include <string_view>

#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

TEST(TextLabelTest, BuildsTextRenderCommandWhenTextIsNotEmpty)
{
	spk::TextLabel label("Label", "Hello");
	label.setGeometry(spk::Rect2D(0, 0, 120, 40));
	label.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	std::shared_ptr<spk::RenderUnit> renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
	EXPECT_TRUE(label.isActivated());
}

TEST(TextLabelTest, NameOnlyConstructorActivates)
{
	spk::TextLabel label("Label");

	EXPECT_TRUE(label.isActivated());
	EXPECT_TRUE(label.text().empty());
}

TEST(TextLabelTest, WithStyleConstructorAppliesStyle)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setTextSize(spk::Font::Size(22, 2));

	spk::TextLabel label("Label", "Hello", style);

	EXPECT_EQ(label.textSize(), spk::Font::Size(22, 2));
	EXPECT_TRUE(label.isActivated());
}

TEST(TextLabelTest, SetFontNullThrows)
{
	spk::TextLabel label("Label", "Hello");

	EXPECT_THROW(label.setFont(nullptr), std::invalid_argument);
}

TEST(TextLabelTest, SetFontUpdatesAndFontGetterReturnsIt)
{
	spk::TextLabel label("Label", "Hello");
	auto font = spk::WidgetStyle::makeDefault().font();

	label.setFont(font);

	EXPECT_EQ(label.font(), font);
}

TEST(TextLabelTest, SetTextSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");
	const auto currentText = label.text();

	label.setText(currentText);

	EXPECT_EQ(label.text(), currentText);
}

TEST(TextLabelTest, SetTextDifferentUpdates)
{
	spk::TextLabel label("Label", "Hello");
	const auto newText = spk::Font::textFromUTF8("World");

	label.setText(newText);

	EXPECT_EQ(label.text(), newText);
}

TEST(TextLabelTest, SetTextStringViewUpdates)
{
	spk::TextLabel label("Label", "Hello");

	label.setText(std::string_view("NewText"));

	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("NewText"));
}

TEST(TextLabelTest, SetDepthSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");

	label.setDepth(0.0f);

	EXPECT_FLOAT_EQ(label.depth(), 0.0f);
}

TEST(TextLabelTest, SetDepthUpdatesValue)
{
	spk::TextLabel label("Label", "Hello");

	label.setDepth(3.5f);

	EXPECT_FLOAT_EQ(label.depth(), 3.5f);
}

TEST(TextLabelTest, SetHorizontalAlignmentSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");

	label.setHorizontalAlignment(spk::HorizontalAlignment::Centered);

	EXPECT_EQ(label.horizontalAlignment(), spk::HorizontalAlignment::Centered);
}

TEST(TextLabelTest, SetHorizontalAlignmentUpdates)
{
	spk::TextLabel label("Label", "Hello");

	label.setHorizontalAlignment(spk::HorizontalAlignment::Right);

	EXPECT_EQ(label.horizontalAlignment(), spk::HorizontalAlignment::Right);
}

TEST(TextLabelTest, SetVerticalAlignmentSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");

	label.setVerticalAlignment(spk::VerticalAlignment::Centered);

	EXPECT_EQ(label.verticalAlignment(), spk::VerticalAlignment::Centered);
}

TEST(TextLabelTest, SetVerticalAlignmentUpdates)
{
	spk::TextLabel label("Label", "Hello");

	label.setVerticalAlignment(spk::VerticalAlignment::Down);

	EXPECT_EQ(label.verticalAlignment(), spk::VerticalAlignment::Down);
}

TEST(TextLabelTest, SetAlignmentBothSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");

	label.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	EXPECT_EQ(label.horizontalAlignment(), spk::HorizontalAlignment::Centered);
	EXPECT_EQ(label.verticalAlignment(), spk::VerticalAlignment::Centered);
}

TEST(TextLabelTest, SetPaddingSameIsNoOp)
{
	spk::TextLabel label("Label", "Hello");
	const auto currentPadding = label.padding();

	label.setPadding(currentPadding);

	EXPECT_EQ(label.padding(), currentPadding);
}

TEST(TextLabelTest, SetPaddingDifferentUpdates)
{
	spk::TextLabel label("Label", "Hello");
	const spk::Vector2Int newPadding = {12, 8};

	label.setPadding(newPadding);

	EXPECT_EQ(label.padding(), newPadding);
}

TEST(TextLabelTest, RenderUnitIsEmptyWhenGeometryIsEmpty)
{
	spk::TextLabel label("Label", "Hello");

	std::shared_ptr<spk::RenderUnit> renderUnit = label.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_TRUE(renderUnit->empty());
}

TEST(TextLabelTest, TextAnchorExercisesAllAlignmentBranches)
{
	spk::TextLabel label("Label", "Hello");
	label.setGeometry(spk::Rect2D(10, 10, 200, 80));

	const spk::HorizontalAlignment horizontalAlignments[] = {
		spk::HorizontalAlignment::Left,
		spk::HorizontalAlignment::Centered,
		spk::HorizontalAlignment::Right
	};
	const spk::VerticalAlignment verticalAlignments[] = {
		spk::VerticalAlignment::Top,
		spk::VerticalAlignment::Centered,
		spk::VerticalAlignment::Down
	};

	for (const auto h : horizontalAlignments)
	{
		for (const auto v : verticalAlignments)
		{
			label.setAlignment(h, v);
			label.invalidateRenderUnit();
			EXPECT_NE(label.renderUnit(), nullptr);
		}
	}
}


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
