#include <gtest/gtest.h>

#include "structures/widget/spk_dynamic_text_label.hpp"
#include "structures/widget/spk_text_edit.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

namespace
{
	spk::WidgetStyle makeTextStyle()
	{
		spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
		style.setTextSize(spk::Font::Size(16, 2));
		style.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
		style.setOutlineColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
		style.setTextPadding({0, 0});
		return style;
	}
}

TEST(DynamicTextLabelVisualTest, RendersProducerTextAfterRefresh)
{
	const spk::Rect2D captureRect(0, 0, 240, 60);

	spk::DynamicTextLabel label("DynLabel");
	label.applyStyle(makeTextStyle());
	label.setTextProducer([]() { return "Hello World"; });
	label.refresh();

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "DynamicTextLabelVisual", "after_refresh", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(DynamicTextLabelVisualTest, RendersEmptyBeforeRefresh)
{
	const spk::Rect2D captureRect(0, 0, 240, 60);

	spk::DynamicTextLabel label("DynLabel");
	label.applyStyle(makeTextStyle());

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "DynamicTextLabelVisual", "before_refresh", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextEditVisualTest, RendersEmptyWithPlaceholder)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setPlaceholder("Enter text here");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "empty_placeholder", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextEditVisualTest, RendersWithText)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setText("Hello World");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "with_text", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextEditVisualTest, RendersObscuredText)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setText("password");
	edit.setObscured(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "obscured", captureRect);

	EXPECT_TRUE(result.matches);
}
