#include <gtest/gtest.h>

#include "structures/widget/spk_command_panel.hpp"
#include "structures/widget/spk_debug_overlay.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(CommandPanelVisualTest, RendersSingleButton)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "CommandPanelVisual", "single_button", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CommandPanelVisualTest, RendersMultipleButtons)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.addButton("yes", "Yes");
	panel.addButton("no", "No");
	panel.addButton("cancel", "Cancel");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "CommandPanelVisual", "three_buttons", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CommandPanelVisualTest, RendersExtendSizePolicy)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.setSizePolicy(spk::Layout::SizePolicy::Extend);
	panel.addButton("ok", "OK");
	panel.addButton("cancel", "Cancel");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "CommandPanelVisual", "extend_policy", captureRect);

	EXPECT_TRUE(result.matches);
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
