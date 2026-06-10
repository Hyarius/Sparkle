#include <gtest/gtest.h>

#include "structures/widget/spk_interface_window.hpp"
#include "structures/widget/spk_numeric_spin_box.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_prompt_panel.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

TEST(InterfaceWindowVisualTest, RendersDefault)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("My Window");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(InterfaceWindowVisualTest, RendersMinimized)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("Minimized Window");
	window.setGeometry(captureRect.atOrigin());
	window.minimize();

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "minimized", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(InterfaceWindowVisualTest, RendersWithoutMinimizeButton)
{
	const spk::Rect2D captureRect(0, 0, 320, 200);

	spk::InterfaceWindow<spk::Panel> window("Window");
	window.setTitle("No Minimize");
	window.deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Minimize);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(window, "InterfaceWindowVisual", "no_minimize_button", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PromptPanelVisualTest, RendersWithMessageAndButton)
{
	const spk::Rect2D captureRect(0, 0, 320, 140);

	spk::PromptPanel panel("Prompt");
	panel.setMessage("Please confirm your action.");
	panel.addButton("ok", "OK");
	panel.addButton("cancel", "Cancel");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "PromptPanelVisual", "message_and_buttons", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(PromptPanelVisualTest, RendersEmptyPrompt)
{
	const spk::Rect2D captureRect(0, 0, 320, 80);

	spk::PromptPanel panel("Prompt");
	panel.addButton("close", "Close");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "PromptPanelVisual", "button_only", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(NumericSpinBoxVisualTest, RendersIntSpinBoxDefault)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::IntSpinBox spinBox("SpinBox");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "int_default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(NumericSpinBoxVisualTest, RendersWithCustomValue)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setValue(42);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "int_value_42", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(NumericSpinBoxVisualTest, RendersFloatSpinBox)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::FloatSpinBox spinBox("SpinBox");
	spinBox.setValue(3.14f);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "float_pi", captureRect);

	EXPECT_TRUE(result.matches);
}
