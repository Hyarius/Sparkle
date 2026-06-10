#include <gtest/gtest.h>

#include <string_view>

#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"

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

	label.setHorizontalAlignment(spk::HorizontalAlignment::Left);

	EXPECT_EQ(label.horizontalAlignment(), spk::HorizontalAlignment::Left);
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

	label.setVerticalAlignment(spk::VerticalAlignment::Top);

	EXPECT_EQ(label.verticalAlignment(), spk::VerticalAlignment::Top);
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

	label.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);

	EXPECT_EQ(label.horizontalAlignment(), spk::HorizontalAlignment::Left);
	EXPECT_EQ(label.verticalAlignment(), spk::VerticalAlignment::Top);
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
