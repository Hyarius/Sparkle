#include <gtest/gtest.h>

#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(DebugOverlayTest, ConfigureRowsCreatesLabels)
{
	spk::DebugOverlay overlay("Overlay");

	overlay.configureRows(2, 3);

	EXPECT_EQ(overlay.nbRow(), 2u);
	EXPECT_EQ(overlay.nbColumn(0), 3u);
	EXPECT_EQ(overlay.nbColumn(1), 3u);
	EXPECT_NE(overlay.label(1, 2), nullptr);
	EXPECT_EQ(overlay.label(2, 0), nullptr);
}

TEST(DebugOverlayTest, SetTextCreatesMissingCells)
{
	spk::DebugOverlay overlay("Overlay");

	overlay.setText(1, 2, "FPS: 60");

	EXPECT_EQ(overlay.nbRow(), 2u);
	EXPECT_EQ(overlay.nbColumn(1), 3u);
	ASSERT_NE(overlay.label(1, 2), nullptr);
	EXPECT_EQ(overlay.label(1, 2)->text(), spk::Font::textFromUTF8("FPS: 60"));
}

TEST(DebugOverlayTest, GeometryDistributesLabels)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(2, 2);
	overlay.setText(0, 0, "TopLeft");
	overlay.setText(1, 1, "BottomRight");

	overlay.setGeometry(spk::Rect2D(0, 0, 400, 100));

	const spk::TextLabel* topLeft = overlay.label(0, 0);
	const spk::TextLabel* bottomRight = overlay.label(1, 1);

	ASSERT_NE(topLeft, nullptr);
	ASSERT_NE(bottomRight, nullptr);
	EXPECT_GT(topLeft->geometry().width(), 0u);
	EXPECT_GT(bottomRight->geometry().y(), topLeft->geometry().y());
	EXPECT_GT(bottomRight->geometry().x(), topLeft->geometry().x());
}

TEST(DebugOverlayTest, SetFontColorAppliesToLabels)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);

	overlay.setFontColor(spk::Color(1.0f, 0.0f, 0.0f, 1.0f), spk::Color(0.0f, 1.0f, 0.0f, 1.0f));

	ASSERT_NE(overlay.label(0, 0), nullptr);
	EXPECT_FLOAT_EQ(overlay.label(0, 0)->glyphColor().r, 1.0f);
	EXPECT_FLOAT_EQ(overlay.label(0, 0)->outlineColor().g, 1.0f);
}

TEST(DebugOverlayTest, SetFontAppliesToExistingLabels)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	auto defaultFont = overlay.label(0, 0)->font();
	auto newFont = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).font();

	overlay.setFont(newFont);

	ASSERT_NE(overlay.label(0, 0), nullptr);
	EXPECT_EQ(overlay.label(0, 0)->font(), newFont);
}

TEST(DebugOverlayTest, SetFontNullFallsBackToDefault)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	auto defaultFont = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).font();

	overlay.setFont(nullptr);

	ASSERT_NE(overlay.label(0, 0), nullptr);
	EXPECT_EQ(overlay.label(0, 0)->font(), defaultFont);
}

TEST(DebugOverlayTest, SetFontOutlineSizeDoesNotCrash)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	overlay.setText(0, 0, "test");
	overlay.setGeometry(spk::Rect2D(0, 0, 200, 40));

	EXPECT_NO_THROW(overlay.setFontOutlineSize(2));
}

TEST(DebugOverlayTest, SetMaxGlyphSizeCapsFontSize)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	overlay.setText(0, 0, "A");
	overlay.setGeometry(spk::Rect2D(0, 0, 200, 40));

	overlay.setMaxGlyphSize(8);

	ASSERT_NE(overlay.label(0, 0), nullptr);
	EXPECT_LE(overlay.label(0, 0)->textSize().glyph, 8u);
}

TEST(DebugOverlayTest, SetRowColumnsReducingRemovesColumns)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 4);
	ASSERT_EQ(overlay.nbColumn(0), 4u);

	overlay.setRowColumns(0, 2);

	EXPECT_EQ(overlay.nbColumn(0), 2u);
}

TEST(DebugOverlayTest, SetRowColumnsNoOpWhenSameCount)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 3);

	overlay.setRowColumns(0, 3);

	EXPECT_EQ(overlay.nbColumn(0), 3u);
}

TEST(DebugOverlayTest, LabelHeightIsZeroWhenEmpty)
{
	spk::DebugOverlay overlay("Overlay");

	EXPECT_EQ(overlay.labelHeight(), 0u);
}

TEST(DebugOverlayTest, LabelHeightIsPositiveAfterGeometry)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	overlay.setText(0, 0, "FPS");
	overlay.setGeometry(spk::Rect2D(0, 0, 200, 50));

	EXPECT_GT(overlay.labelHeight(), 0u);
}

TEST(DebugOverlayTest, NbColumnOutOfRangeReturnsZero)
{
	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(2, 3);

	EXPECT_EQ(overlay.nbColumn(5), 0u);
}

TEST(DebugOverlayVisualTest, RendersSingleCell)
{
	const spk::Rect2D captureRect(0, 0, 300, 80);

	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	overlay.setText(0, 0, "FPS: 60");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(overlay, "DebugOverlayVisual", "single_cell", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(DebugOverlayVisualTest, RendersGrid)
{
	const spk::Rect2D captureRect(0, 0, 300, 120);

	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(2, 2);
	overlay.setText(0, 0, "FPS: 60");
	overlay.setText(0, 1, "CPU: 12%");
	overlay.setText(1, 0, "Mem: 256MB");
	overlay.setText(1, 1, "GPU: 45%");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(overlay, "DebugOverlayVisual", "two_by_two_grid", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(DebugOverlayVisualTest, RendersWithCustomColor)
{
	const spk::Rect2D captureRect(0, 0, 300, 60);

	spk::DebugOverlay overlay("Overlay");
	overlay.configureRows(1, 1);
	overlay.setFontColor(spk::Color(1.0f, 1.0f, 0.0f, 1.0f), spk::Color(0.5f, 0.0f, 0.0f, 1.0f));
	overlay.setText(0, 0, "Warning!");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(overlay, "DebugOverlayVisual", "yellow_text", captureRect);

	EXPECT_TRUE(result.matches);
}
