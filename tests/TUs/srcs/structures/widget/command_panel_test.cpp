#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_command_panel.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(CommandPanelTest, AddButtonRegistersButton)
{
	spk::CommandPanel panel("Panel");

	spk::PushButton* button = panel.addButton("ok", "OK");

	ASSERT_NE(button, nullptr);
	EXPECT_EQ(panel.nbButton(), 1u);
	EXPECT_EQ(panel.button("ok"), button);
	EXPECT_EQ(button->releasedLabel().text(), spk::Font::textFromUTF8("OK"));
}

TEST(CommandPanelTest, AddDuplicateButtonThrows)
{
	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");

	EXPECT_THROW(panel.addButton("ok", "OK"), std::runtime_error);
}

TEST(CommandPanelTest, UnknownButtonThrows)
{
	spk::CommandPanel panel("Panel");

	EXPECT_THROW((void)panel.button("missing"), std::runtime_error);
}

TEST(CommandPanelTest, RemoveButtonErasesIt)
{
	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");

	panel.removeButton("ok");

	EXPECT_EQ(panel.nbButton(), 0u);
	EXPECT_THROW((void)panel.button("ok"), std::runtime_error);
}

TEST(CommandPanelTest, ButtonsArePackedToTheRightByDefault)
{
	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");
	panel.addButton("cancel", "Cancel");

	panel.setGeometry(spk::Rect2D(0, 0, 400, 40));

	const spk::PushButton* okButton = panel.button("ok");
	const spk::PushButton* cancelButton = panel.button("cancel");

	EXPECT_GT(okButton->geometry().x(), 0);
	EXPECT_GT(cancelButton->geometry().x(), okButton->geometry().x());
	EXPECT_EQ(static_cast<unsigned int>(cancelButton->geometry().x()) + cancelButton->geometry().width(), 400u);
}

TEST(CommandPanelTest, SubscribeTriggersOnButtonClick)
{
	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");
	panel.setGeometry(spk::Rect2D(0, 0, 400, 40));

	int clickCount = 0;
	auto contract = panel.subscribe("ok", [&clickCount]() { ++clickCount; });

	const spk::Rect2D buttonRect = panel.button("ok")->geometry();
	const spk::Vector2Int clickPosition = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2), buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&panel);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(clickCount, 1);
}

TEST(CommandPanelTest, MinimalSizeAccountsForButtonsAndPadding)
{
	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");
	panel.addButton("cancel", "Cancel");

	const spk::Vector2UInt minimalSize = panel.minimalSize();

	EXPECT_GT(minimalSize.x, 0u);
	EXPECT_GT(minimalSize.y, 0u);
}

TEST(CommandPanelVisualTest, RendersSingleButton)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.addButton("ok", "OK");

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(panel, "CommandPanelVisual", "single_button", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CommandPanelVisualTest, RendersMultipleButtons)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.addButton("yes", "Yes");
	panel.addButton("no", "No");
	panel.addButton("cancel", "Cancel");

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(panel, "CommandPanelVisual", "three_buttons", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(CommandPanelVisualTest, RendersExtendSizePolicy)
{
	const spk::Rect2D captureRect(0, 0, 300, 48);

	spk::CommandPanel panel("Panel");
	panel.setSizePolicy(spk::Layout::SizePolicy::Extend);
	panel.addButton("ok", "OK");
	panel.addButton("cancel", "Cancel");

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(panel, "CommandPanelVisual", "extend_policy", captureRect);

	EXPECT_TRUE(result.matches);
}
