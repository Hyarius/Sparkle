#include <gtest/gtest.h>

#include "structures/widget/spk_prompt_panel.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

TEST(PromptPanelTest, SetMessageUpdatesTextArea)
{
	spk::PromptPanel panel("Prompt");

	panel.setMessage("Are you sure?");

	EXPECT_EQ(panel.message(), spk::Font::textFromUTF8("Are you sure?"));
	EXPECT_EQ(panel.textArea().text(), spk::Font::textFromUTF8("Are you sure?"));
}

TEST(PromptPanelTest, TextAreaHasNoBackground)
{
	spk::PromptPanel panel("Prompt");

	EXPECT_FALSE(panel.textArea().isBackgroundVisible());
}

TEST(PromptPanelTest, AddButtonRegistersInCommandPanel)
{
	spk::PromptPanel panel("Prompt");

	spk::PushButton* button = panel.addButton("ok", "OK");

	ASSERT_NE(button, nullptr);
	EXPECT_EQ(panel.button("ok"), button);
	EXPECT_EQ(panel.commandPanel().nbButton(), 1u);

	panel.removeButton("ok");
	EXPECT_EQ(panel.commandPanel().nbButton(), 0u);
}

TEST(PromptPanelTest, MinimalSizeGrowsWithMessageAndButtons)
{
	spk::PromptPanel panel("Prompt");

	const spk::Vector2UInt emptySize = panel.minimalSize();

	panel.setMessage("Some long message that needs space to be displayed");
	panel.addButton("ok", "OK");

	const spk::Vector2UInt filledSize = panel.minimalSize();

	EXPECT_GT(filledSize.x, emptySize.x);
	EXPECT_GT(filledSize.y, emptySize.y);
}

TEST(PromptPanelTest, SubscribeTriggersOnButtonClick)
{
	spk::PromptPanel panel("Prompt");
	panel.setMessage("Proceed?");
	panel.addButton("ok", "OK");
	panel.setGeometry(spk::Rect2D(0, 0, 400, 200));

	int clickCount = 0;
	auto contract = panel.subscribe("ok", [&clickCount]() { ++clickCount; });

	const spk::Rect2D buttonRect = panel.button("ok")->viewport().geometry();
	const spk::Vector2Int clickPosition = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&panel);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(clickCount, 1);
}

TEST(PromptPanelTest, SetButtonSizePolicyForwardsToCommandPanel)
{
	spk::PromptPanel panel("Prompt");

	panel.setButtonSizePolicy(spk::Layout::SizePolicy::Extend);

	EXPECT_EQ(panel.buttonSizePolicy(), spk::Layout::SizePolicy::Extend);
}

TEST(PromptPanelTest, SetButtonPaddingForwardsToCommandPanel)
{
	spk::PromptPanel panel("Prompt");
	panel.addButton("ok", "OK");

	const spk::Vector2UInt sizeBefore = panel.commandPanel().minimalSize();
	panel.setButtonPadding(spk::Vector2UInt(40, 20));
	const spk::Vector2UInt sizeAfter = panel.commandPanel().minimalSize();

	EXPECT_GE(sizeAfter.x, sizeBefore.x);
}

TEST(PromptPanelTest, ConstAccessorsReturnSameObjects)
{
	spk::PromptPanel panel("Prompt");
	const spk::PromptPanel& cpanel = panel;

	EXPECT_EQ(&cpanel.background(), &panel.background());
	EXPECT_EQ(&cpanel.textArea(), &panel.textArea());
	EXPECT_EQ(&cpanel.commandPanel(), &panel.commandPanel());
}

TEST(PromptPanelTest, ButtonGetterReturnsAddedButton)
{
	spk::PromptPanel panel("Prompt");
	spk::PushButton* added = panel.addButton("ok", "OK");

	EXPECT_EQ(panel.button("ok"), added);
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
