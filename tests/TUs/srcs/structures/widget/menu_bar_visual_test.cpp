#include <gtest/gtest.h>

#include "structures/widget/spk_action_bar.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

// Included last so its MessageBox macro #undef wins over <Windows.h>
#include "structures/widget/spk_message_box.hpp"

TEST(MenuBarVisualTest, RendersBareBar)
{
	const spk::Rect2D captureRect(0, 0, 400, 28);

	spk::MenuBar bar("MenuBar");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "MenuBarVisual", "bare", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(MenuBarVisualTest, RendersWithMenuButtons)
{
	const spk::Rect2D captureRect(0, 0, 400, 28);

	spk::MenuBar bar("MenuBar");
	bar.addMenu("File");
	bar.addMenu("Edit");
	bar.addMenu("View");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "MenuBarVisual", "three_menus", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(MenuBarVisualTest, RendersOpenMenu)
{
	const spk::Rect2D captureRect(0, 0, 400, 160);

	spk::MenuBar bar("MenuBar");
	spk::MenuBar::Menu* menu = bar.addMenu("File");
	menu->addItem("New", []() {});
	menu->addItem("Open", []() {});
	menu->addBreak();
	menu->addItem("Quit", []() {});
	menu->activate();
	bar.setGeometry(captureRect.atOrigin());

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(bar, "MenuBarVisual", "open_menu", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(MessageBoxVisualTest, RendersWithText)
{
	const spk::Rect2D captureRect(0, 0, 400, 200);

	spk::MessageBox box("MessageBox");
	box.setText("Something went wrong. Please try again.");
	box.addButton("ok", "OK");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(box, "MessageBoxVisual", "with_text", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(InformationMessageBoxVisualTest, RendersDefault)
{
	const spk::Rect2D captureRect(0, 0, 400, 200);

	spk::InformationMessageBox box("InfoBox");
	box.setText("Operation completed successfully.");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(box, "InformationMessageBoxVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(RequestMessageBoxVisualTest, RendersDefault)
{
	const spk::Rect2D captureRect(0, 0, 400, 200);

	spk::RequestMessageBox box("RequestBox");
	box.setText("Are you sure you want to proceed?");
	box.configure("Yes", []() {}, "No", []() {});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(box, "RequestMessageBoxVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}
