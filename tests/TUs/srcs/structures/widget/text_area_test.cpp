#include <gtest/gtest.h>

#include <limits>
#include <stdexcept>

#include "structures/widget/spk_form_layout.hpp"
#include "structures/widget/spk_grid_layout.hpp"
#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_text_area.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"

TEST(TextAreaTest, NameOnlyConstructorActivates)
{
	spk::TextArea textArea("TextArea");

	EXPECT_TRUE(textArea.isActivated());
	EXPECT_TRUE(textArea.text().empty());
	EXPECT_EQ(textArea.horizontalAlignment(), spk::HorizontalAlignment::Left);
	EXPECT_EQ(textArea.verticalAlignment(), spk::VerticalAlignment::Top);
}

TEST(TextAreaTest, EmptyTextRendersNoCommands)
{
	spk::TextArea textArea("TextArea");
	textArea.setGeometry(spk::Rect2D(0, 0, 200, 100));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 0u);
}

TEST(TextAreaTest, SingleLineRendersOneTextCommand)
{
	spk::TextArea textArea("TextArea", "Hello");
	textArea.setGeometry(spk::Rect2D(0, 0, 400, 100));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 1u);
}

TEST(TextAreaTest, ExplicitNewLinesProduceOneCommandPerLine)
{
	spk::TextArea textArea("TextArea", "first\nsecond\nthird");
	textArea.setGeometry(spk::Rect2D(0, 0, 400, 200));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 3u);
}

TEST(TextAreaTest, LongTextWrapsIntoSeveralLines)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setGeometry(spk::Rect2D(0, 0, 120, 300));

	auto renderUnit = textArea.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_GT(renderUnit->size(), 1u);
}

TEST(TextAreaTest, ComputeMinimalSizeGrowsWithNarrowerWidth)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");

	const spk::Vector2UInt wideSize = textArea.computePreferredSize(600);
	const spk::Vector2UInt narrowSize = textArea.computePreferredSize(120);

	EXPECT_GT(narrowSize.y, wideSize.y);
}

TEST(TextAreaTest, SetMinimalWidthGrowsComputedMinimalWidth)
{
	spk::TextArea textArea("TextArea", "short");

	textArea.setMinimalWidth(300);

	EXPECT_GE(textArea.computePreferredSize(10).x, 300u);
}

TEST(TextAreaTest, SetMinimalWidthGrowsWidgetMinimalSize)
{
	spk::TextArea textArea("TextArea", "short");

	textArea.setMinimalWidth(300);

	EXPECT_GE(textArea.minimalSize().x, 300u);
}

TEST(TextAreaTest, MinimalSizeUsesMinimalWidthForWrappedHeight)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");

	textArea.setMinimalWidth(80);

	EXPECT_EQ(textArea.minimalSize(), textArea.computePreferredSize(80));
	EXPECT_GT(textArea.minimalSize().y, textArea.computePreferredSize(600).y);
}

TEST(TextAreaTest, PreferredSizeForUsesProvidedAvailableWidth)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");

	const spk::Vector2UInt constrained =
		textArea.preferredSizeFor({120, std::numeric_limits<spk::Vector2UInt::value_type>::max()});
	const spk::Vector2UInt wide =
		textArea.preferredSizeFor({600, std::numeric_limits<spk::Vector2UInt::value_type>::max()});

	EXPECT_EQ(constrained, textArea.computePreferredSize(120));
	EXPECT_EQ(wide, textArea.computePreferredSize(600));
	EXPECT_GT(constrained.y, wide.y);
}

TEST(TextAreaTest, SetMinimalWidthOverridesGenericMinimalSize)
{
	spk::TextArea textArea("TextArea", "short");
	textArea.setMinimalSize({1, 1});

	textArea.setMinimalWidth(300);

	EXPECT_GE(textArea.minimalSize().x, 300u);
}

TEST(TextAreaTest, SetMinimalWidthZeroRestoresComputedMinimalSize)
{
	spk::TextArea textArea("TextArea", "short");
	textArea.setMinimalSize({1, 1});

	textArea.setMinimalWidth(0);

	EXPECT_GT(textArea.minimalSize().x, 1u);
	EXPECT_GT(textArea.minimalSize().y, 1u);
}

TEST(TextAreaTest, GridLayoutUsesAllocatedWidthForWrappedMinimalHeight)
{
	spk::Widget wideWidget("WideWidget");
	wideWidget.setMinimalSize({400, 20});

	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(80);

	spk::GridLayout layout;
	layout.setWidget(0, 0, &wideWidget, spk::Layout::SizePolicy::Minimum);
	layout.setWidget(0, 1, &textArea, spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 400, 1000));

	EXPECT_EQ(textArea.geometry().height(), textArea.computePreferredSize(400).y);
	EXPECT_LT(textArea.geometry().height(), textArea.computePreferredSize(80).y);
}

TEST(TextAreaTest, GridLayoutUsesMaximalWidthForWrappedMinimalHeight)
{
	spk::Widget wideWidget("WideWidget");
	wideWidget.setMinimalSize({400, 20});

	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);
	textArea.setMaximalSize({300, std::numeric_limits<spk::Vector2UInt::value_type>::max()});

	spk::GridLayout layout;
	layout.setWidget(0, 0, &wideWidget, spk::Layout::SizePolicy::Minimum);
	layout.setWidget(0, 1, &textArea, spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 400, 1000));

	EXPECT_EQ(textArea.geometry().width(), 300u);
	EXPECT_EQ(textArea.geometry().height(), textArea.computePreferredSize(300).y);
}

TEST(TextAreaTest, VerticalLayoutUsesAllocatedWidthForWrappedMinimalHeight)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);

	spk::VerticalLayout layout;
	layout.addWidget(&textArea, spk::Layout::SizePolicy::Minimum);

	const spk::Vector2UInt preferredSize =
		layout.preferredSizeFor({400, std::numeric_limits<spk::Vector2UInt::value_type>::max()});
	layout.setGeometry(spk::Rect2D(0, 0, 400, preferredSize.y));

	EXPECT_EQ(preferredSize.y, textArea.computePreferredSize(400).y);
	EXPECT_EQ(textArea.geometry().width(), 400u);
	EXPECT_EQ(textArea.geometry().height(), textArea.computePreferredSize(400).y);
	EXPECT_LT(textArea.geometry().height(), textArea.computePreferredSize(80).y);
}

TEST(TextAreaTest, FormLayoutUsesAllocatedFieldWidthForWrappedMinimalHeight)
{
	spk::Widget label("Label");
	label.setMinimalSize({100, 20});

	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);

	spk::FormLayout layout;
	layout.addRow(&label, &textArea);

	const spk::Vector2UInt preferredSize =
		layout.preferredSizeFor({400, std::numeric_limits<spk::Vector2UInt::value_type>::max()});
	layout.setGeometry(spk::Rect2D(0, 0, 400, preferredSize.y));

	EXPECT_EQ(preferredSize.y, textArea.computePreferredSize(300).y);
	EXPECT_EQ(textArea.geometry().width(), 300u);
	EXPECT_EQ(textArea.geometry().height(), textArea.computePreferredSize(300).y);
	EXPECT_LT(textArea.geometry().height(), textArea.computePreferredSize(80).y);
}

TEST(TextAreaTest, GridLayoutMinimalWidthUsesNarrowestNotUnwrappedWidth)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);

	spk::GridLayout layout;
	layout.setWidget(0, 0, &textArea, spk::Layout::SizePolicy::Minimum);

	EXPECT_EQ(layout.minimalSize().x, textArea.minimalSize().x);
	EXPECT_LT(layout.minimalSize().x, textArea.computePreferredSize(600).x);
}

TEST(TextAreaTest, VerticalLayoutMinimalWidthUsesNarrowestNotUnwrappedWidth)
{
	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);

	spk::VerticalLayout layout;
	layout.addWidget(&textArea, spk::Layout::SizePolicy::Minimum);

	EXPECT_EQ(layout.minimalSize().x, textArea.minimalSize().x);
	EXPECT_LT(layout.minimalSize().x, textArea.computePreferredSize(600).x);
}

TEST(TextAreaTest, FormLayoutMinimalWidthUsesNarrowestNotUnwrappedFieldWidth)
{
	spk::Widget label("Label");
	label.setMinimalSize({100, 20});

	spk::TextArea textArea("TextArea", "one two three four five six seven eight nine ten");
	textArea.setMinimalWidth(0);

	spk::FormLayout layout;
	layout.addRow(&label, &textArea);

	EXPECT_EQ(layout.minimalSize().x, label.minimalSize().x + textArea.minimalSize().x);
	EXPECT_LT(layout.minimalSize().x, label.minimalSize().x + textArea.computePreferredSize(600).x);
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

TEST(TextAreaTest, SetFontRejectsNull)
{
	spk::TextArea textArea("TextArea");

	EXPECT_THROW(textArea.setFont(nullptr), std::invalid_argument);
}

TEST(TextAreaTest, StyleEditionRefreshesProperties)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextArea textArea("TextArea", "Hello", style);

	style.setTextSize(spk::Font::Size(28, 1));

	EXPECT_EQ(textArea.textSize(), spk::Font::Size(28, 1));
}
