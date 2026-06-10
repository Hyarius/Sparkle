#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_text_area.hpp"
#include "structures/widget/spk_widget_style.hpp"

TEST(TextAreaTest, NameOnlyConstructorActivates)
{
	spk::TextArea textArea("TextArea");

	EXPECT_TRUE(textArea.isActivated());
	EXPECT_TRUE(textArea.text().empty());
	EXPECT_EQ(textArea.horizontalAlignment(), spk::HorizontalAlignment::Left);
	EXPECT_EQ(textArea.verticalAlignment(), spk::VerticalAlignment::Top);
}

TEST(TextAreaTest, EmptyTextRendersOnlyBackground)
{
	spk::TextArea textArea("TextArea");
	textArea.setGeometry(spk::Rect2D(0, 0, 200, 100));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
}

TEST(TextAreaTest, SingleLineRendersBackgroundAndOneText)
{
	spk::TextArea textArea("TextArea", "Hello");
	textArea.setGeometry(spk::Rect2D(0, 0, 400, 100));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 2u);
}

TEST(TextAreaTest, ExplicitNewLinesProduceOneCommandPerLine)
{
	spk::TextArea textArea("TextArea", "first\nsecond\nthird");
	textArea.setGeometry(spk::Rect2D(0, 0, 400, 200));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 4u);
}

TEST(TextAreaTest, LongTextWrapsIntoSeveralLines)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setGeometry(spk::Rect2D(0, 0, 120, 300));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_GT(renderUnit->size(), 2u);
}

TEST(TextAreaTest, ComputeMinimalSizeGrowsWithNarrowerWidth)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");

	const spk::Vector2UInt wideSize = textArea.computeMinimalSize(600);
	const spk::Vector2UInt narrowSize = textArea.computeMinimalSize(120);

	EXPECT_GT(narrowSize.y, wideSize.y);
}

TEST(TextAreaTest, SetTextUpdatesValue)
{
	spk::TextArea textArea("TextArea");

	textArea.setText("Updated");

	EXPECT_EQ(textArea.text(), spk::Font::textFromUTF8("Updated"));
}

TEST(TextAreaTest, SetLinePaddingUpdatesValue)
{
	spk::TextArea textArea("TextArea");

	textArea.setLinePadding(4);

	EXPECT_EQ(textArea.linePadding(), 4u);
}

TEST(TextAreaTest, SetAlignmentUpdatesValues)
{
	spk::TextArea textArea("TextArea");

	textArea.setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);

	EXPECT_EQ(textArea.horizontalAlignment(), spk::HorizontalAlignment::Centered);
	EXPECT_EQ(textArea.verticalAlignment(), spk::VerticalAlignment::Centered);
}

TEST(TextAreaTest, SetSpriteSheetRejectsNull)
{
	spk::TextArea textArea("TextArea");

	EXPECT_THROW(textArea.setSpriteSheet(nullptr), std::invalid_argument);
}

TEST(TextAreaTest, SetFontRejectsNull)
{
	spk::TextArea textArea("TextArea");

	EXPECT_THROW(textArea.setFont(nullptr), std::invalid_argument);
}

TEST(TextAreaTest, SetCornerSizeRejectsNegativeComponents)
{
	spk::TextArea textArea("TextArea");

	EXPECT_THROW(textArea.setCornerSize({-1, 0}), std::invalid_argument);
}

TEST(TextAreaTest, StyleEditionRefreshesProperties)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextArea textArea("TextArea", "Hello", style);

	style.setTextSize(spk::Font::Size(28, 1));

	EXPECT_EQ(textArea.textSize(), spk::Font::Size(28, 1));
}
