#include <gtest/gtest.h>

#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

#include "structures/widget/spk_message_box.hpp"

TEST(MessageBoxTest, SetTextUpdatesTextArea)
{
	spk::MessageBox messageBox("MessageBox");

	messageBox.setText("Hello world");

	EXPECT_EQ(messageBox.text(), spk::Font::textFromUTF8("Hello world"));
	EXPECT_EQ(messageBox.textArea().text(), spk::Font::textFromUTF8("Hello world"));
}

TEST(MessageBoxTest, AddButtonRegistersInCommandPanel)
{
	spk::MessageBox messageBox("MessageBox");

	spk::PushButton *button = messageBox.addButton("ok", "OK");

	ASSERT_NE(button, nullptr);
	EXPECT_EQ(messageBox.button("ok"), button);
	EXPECT_EQ(messageBox.commandPanel().nbButton(), 1u);

	messageBox.removeButton("ok");
	EXPECT_EQ(messageBox.commandPanel().nbButton(), 0u);
}

TEST(MessageBoxTest, SetMinimalWidthGrowsMinimalSize)
{
	spk::MessageBox messageBox("MessageBox");
	messageBox.setText("Some message content");

	messageBox.setMinimalWidth(400);
	messageBox.setGeometry(spk::Rect2D(0, 0, 10, 10));

	EXPECT_GE(messageBox.geometry().width(), 400u);
}

TEST(MessageBoxTest, UsesInterfaceWindowContentPaddingAndPlacesButtonsBottomRight)
{
	spk::MessageBox messageBox("MessageBox");
	messageBox.setText("Something went wrong. Please try again.");
	messageBox.addButton("ok", "OK");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 200));

	const spk::IInterfaceWindow::ContentPadding padding = messageBox.contentPadding();
	const spk::Rect2D buttonRect = messageBox.button("ok")->viewport().geometry();

	EXPECT_EQ(messageBox.backgroundFrame().cornerSize(), spk::Vector2Int(8, 8));
	EXPECT_EQ(padding, (spk::IInterfaceWindow::ContentPadding{10, 0, 10, 10}));
	EXPECT_EQ(buttonRect.right(), static_cast<int>(messageBox.geometry().width() - padding.right));
	EXPECT_EQ(buttonRect.bottom(), static_cast<int>(messageBox.geometry().height() - padding.bottom));
}

TEST(InformationMessageBoxTest, HasCloseButtonThatDeactivates)
{
	spk::InformationMessageBox messageBox("InfoBox");
	messageBox.setText("Information message");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 300));

	ASSERT_NE(messageBox.button(), nullptr);
	EXPECT_EQ(messageBox.menuBar().titleLabel().text(), spk::Font::textFromUTF8("Information"));

	const spk::Rect2D buttonRect = messageBox.button()->viewport().geometry();
	const spk::Vector2Int clickPosition = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&messageBox);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(messageBox.isActivated());
}

TEST(RequestMessageBoxTest, ConfigureSetsCaptionsAndActions)
{
	spk::RequestMessageBox messageBox("RequestBox");
	messageBox.setText("Proceed?");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 300));

	int acceptCount = 0;
	int declineCount = 0;
	messageBox.configure(
		"Accept",
		[&acceptCount]() {
			++acceptCount;
		},
		"Decline",
		[&declineCount]() {
			++declineCount;
		});

	EXPECT_EQ(messageBox.firstButton()->releasedLabel().text(), spk::Font::textFromUTF8("Accept"));
	EXPECT_EQ(messageBox.secondButton()->releasedLabel().text(), spk::Font::textFromUTF8("Decline"));

	const spk::Rect2D buttonRect = messageBox.firstButton()->viewport().geometry();
	const spk::Vector2Int clickPosition = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&messageBox);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPosition})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(acceptCount, 1);
	EXPECT_EQ(declineCount, 0);
	EXPECT_FALSE(messageBox.isActivated());
}

TEST(MessageBoxTest, SubscribeTriggersButtonClick)
{
	spk::MessageBox messageBox("MessageBox");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 300));
	messageBox.addButton("yes", "Yes");

	int clickCount = 0;
	auto contract = messageBox.subscribe("yes", [&clickCount]() {
		++clickCount;
	});

	const spk::Rect2D buttonRect = messageBox.button("yes")->viewport().geometry();
	const spk::Vector2Int clickPos = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&messageBox);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(clickCount, 1);
}

TEST(MessageBoxTest, ConstAccessorsReturnSameObjects)
{
	spk::MessageBox messageBox("MessageBox");
	const spk::MessageBox &cmb = messageBox;

	EXPECT_EQ(&cmb.content(), &messageBox.content());
	EXPECT_EQ(&cmb.textArea(), &messageBox.textArea());
	EXPECT_EQ(&cmb.commandPanel(), &messageBox.commandPanel());

	messageBox.addButton("ok", "OK");
	EXPECT_EQ(cmb.button("ok"), messageBox.button("ok"));
	EXPECT_EQ(cmb.text(), messageBox.text());
}

TEST(RequestMessageBoxTest, SecondButtonTriggersDeclineAndCloses)
{
	spk::RequestMessageBox messageBox("RequestBox");
	messageBox.setText("Proceed?");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 300));

	int declineCount = 0;
	messageBox.configure(
		"Yes",
		[]() {
		},
		"No",
		[&declineCount]() {
			++declineCount;
		});

	const spk::Rect2D buttonRect = messageBox.secondButton()->viewport().geometry();
	const spk::Vector2Int clickPos = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&messageBox);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_EQ(declineCount, 1);
	EXPECT_FALSE(messageBox.isActivated());
}

TEST(RequestMessageBoxTest, NullActionsInConfigureAreHandledSafely)
{
	spk::RequestMessageBox messageBox("RequestBox");
	messageBox.setGeometry(spk::Rect2D(0, 0, 400, 300));

	EXPECT_NO_THROW(messageBox.configure("Yes", nullptr, "No", nullptr));

	const spk::Rect2D buttonRect = messageBox.firstButton()->viewport().geometry();
	const spk::Vector2Int clickPos = {
		buttonRect.x() + static_cast<int>(buttonRect.width() / 2),
		buttonRect.y() + static_cast<int>(buttonRect.height() / 2)};

	spk::MouseModule mouseModule;
	mouseModule.bind(&messageBox);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = clickPos})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();

	EXPECT_FALSE(messageBox.isActivated());
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
	box.configure("Yes", []() {
	},
				  "No",
				  []() {
				  });

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(box, "RequestMessageBoxVisual", "default", captureRect);

	EXPECT_TRUE(result.matches);
}
