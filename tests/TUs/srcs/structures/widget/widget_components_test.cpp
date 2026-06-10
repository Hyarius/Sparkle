#include <gtest/gtest.h>

#include "structures/widget/spk_widget.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	class WidgetFocusTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			spk::Widget cleanup("__cleanup__");
			cleanup.takeAllFocus();
		}
	};

	spk::KeyboardEventRecord makeKeyPress()
	{
		return spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
			.key = spk::Keyboard::Return,
			.isRepeated = false}));
	}

	spk::MouseEventRecord makeMousePress()
	{
		return spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
			.button = spk::Mouse::Left}));
	}
}

// --- Static focus management ---

TEST_F(WidgetFocusTest, TakeFocusKeyboardSetsGlobalSlot)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeFocus(spk::Widget::FocusType::Keyboard);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &widget);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), nullptr);
}

TEST_F(WidgetFocusTest, TakeFocusMouseSetsGlobalSlot)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeFocus(spk::Widget::FocusType::Mouse);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), &widget);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), nullptr);
}

TEST_F(WidgetFocusTest, TakeFocusTransfersOwnershipFromPreviousHolder)
{
	sparkle_test::RecordingWidget first("First");
	sparkle_test::RecordingWidget second("Second");

	first.takeFocus(spk::Widget::FocusType::Keyboard);
	ASSERT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &first);

	second.takeFocus(spk::Widget::FocusType::Keyboard);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &second);
	EXPECT_FALSE(first.hasFocus(spk::Widget::FocusType::Keyboard));
}

TEST_F(WidgetFocusTest, ReleaseFocusKeyboardClearsSlot)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeFocus(spk::Widget::FocusType::Keyboard);
	widget.releaseFocus(spk::Widget::FocusType::Keyboard);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), nullptr);
}

TEST_F(WidgetFocusTest, ReleaseFocusMouseClearsSlot)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeFocus(spk::Widget::FocusType::Mouse);
	widget.releaseFocus(spk::Widget::FocusType::Mouse);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), nullptr);
}

TEST_F(WidgetFocusTest, ReleaseFocusIgnoresNonOwner)
{
	sparkle_test::RecordingWidget owner("Owner");
	sparkle_test::RecordingWidget other("Other");

	owner.takeFocus(spk::Widget::FocusType::Keyboard);
	other.releaseFocus(spk::Widget::FocusType::Keyboard);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &owner);
}

TEST_F(WidgetFocusTest, HasFocusReturnsTrueForOwnerFalseForOthers)
{
	sparkle_test::RecordingWidget owner("Owner");
	sparkle_test::RecordingWidget other("Other");

	owner.takeFocus(spk::Widget::FocusType::Keyboard);

	EXPECT_TRUE(owner.hasFocus(spk::Widget::FocusType::Keyboard));
	EXPECT_FALSE(other.hasFocus(spk::Widget::FocusType::Keyboard));
	EXPECT_FALSE(owner.hasFocus(spk::Widget::FocusType::Mouse));
}

TEST_F(WidgetFocusTest, TakeAllFocusSetsKeyboardAndMouse)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeAllFocus();

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &widget);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), &widget);
}

TEST_F(WidgetFocusTest, ReleaseAllFocusClearsBothTypes)
{
	sparkle_test::RecordingWidget widget("Widget");

	widget.takeAllFocus();
	widget.releaseAllFocus();

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), nullptr);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), nullptr);
}

TEST_F(WidgetFocusTest, DestructorAutomaticallyReleasesFocus)
{
	{
		sparkle_test::RecordingWidget widget("Widget");
		widget.takeAllFocus();

		ASSERT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &widget);
		ASSERT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), &widget);
	}

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), nullptr);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), nullptr);
}

// --- Event routing with keyboard focus ---

TEST_F(WidgetFocusTest, KeyboardFocusRoutesEventToFocusedWidget)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget focused("Focused", &root);

	root.activate();
	focused.activate();

	focused.takeFocus(spk::Widget::FocusType::Keyboard);

	spk::KeyboardEventRecord event = makeKeyPress();
	sparkle_test::sendKeyboardEvent(root, event);

	EXPECT_EQ(focused.keyboardEventCount, 1);
	EXPECT_EQ(root.keyboardEventCount, 0);
}

TEST_F(WidgetFocusTest, KeyboardFocusAlsoRoutesToDescendantsOfFocusedWidget)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget focused("Focused", &root);
	sparkle_test::RecordingWidget child("Child", &focused);

	root.activate();
	focused.activate();
	child.activate();

	focused.takeFocus(spk::Widget::FocusType::Keyboard);

	spk::KeyboardEventRecord event = makeKeyPress();
	sparkle_test::sendKeyboardEvent(root, event);

	EXPECT_EQ(child.keyboardEventCount, 1);
	EXPECT_EQ(focused.keyboardEventCount, 1);
	EXPECT_EQ(root.keyboardEventCount, 0);
}

TEST_F(WidgetFocusTest, KeyboardFocusBypassesSiblingWidgets)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget sibling("Sibling", &root);
	sparkle_test::RecordingWidget focused("Focused", &root);

	root.activate();
	sibling.activate();
	focused.activate();

	focused.takeFocus(spk::Widget::FocusType::Keyboard);

	spk::KeyboardEventRecord event = makeKeyPress();
	sparkle_test::sendKeyboardEvent(root, event);

	EXPECT_EQ(focused.keyboardEventCount, 1);
	EXPECT_EQ(sibling.keyboardEventCount, 0);
	EXPECT_EQ(root.keyboardEventCount, 0);
}

TEST_F(WidgetFocusTest, WithoutKeyboardFocusEventsPropagateThroughAllWidgets)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget child("Child", &root);

	root.activate();
	child.activate();

	spk::KeyboardEventRecord event = makeKeyPress();
	sparkle_test::sendKeyboardEvent(root, event);

	EXPECT_EQ(root.keyboardEventCount, 1);
	EXPECT_EQ(child.keyboardEventCount, 1);
}

// --- Event routing with mouse focus ---

TEST_F(WidgetFocusTest, MouseFocusRoutesEventToFocusedWidget)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget focused("Focused", &root);

	root.activate();
	focused.activate();

	focused.takeFocus(spk::Widget::FocusType::Mouse);

	spk::MouseEventRecord event = makeMousePress();
	sparkle_test::sendMouseEvent(root, event);

	EXPECT_EQ(focused.mouseEventCount, 1);
	EXPECT_EQ(root.mouseEventCount, 0);
}

TEST_F(WidgetFocusTest, MouseFocusAlsoRoutesToDescendantsOfFocusedWidget)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget focused("Focused", &root);
	sparkle_test::RecordingWidget child("Child", &focused);

	root.activate();
	focused.activate();
	child.activate();

	focused.takeFocus(spk::Widget::FocusType::Mouse);

	spk::MouseEventRecord event = makeMousePress();
	sparkle_test::sendMouseEvent(root, event);

	EXPECT_EQ(child.mouseEventCount, 1);
	EXPECT_EQ(focused.mouseEventCount, 1);
	EXPECT_EQ(root.mouseEventCount, 0);
}

TEST_F(WidgetFocusTest, MouseFocusBypassesSiblingWidgets)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget sibling("Sibling", &root);
	sparkle_test::RecordingWidget focused("Focused", &root);

	root.activate();
	sibling.activate();
	focused.activate();

	focused.takeFocus(spk::Widget::FocusType::Mouse);

	spk::MouseEventRecord event = makeMousePress();
	sparkle_test::sendMouseEvent(root, event);

	EXPECT_EQ(focused.mouseEventCount, 1);
	EXPECT_EQ(sibling.mouseEventCount, 0);
	EXPECT_EQ(root.mouseEventCount, 0);
}

TEST_F(WidgetFocusTest, WithoutMouseFocusEventsPropagateThroughAllWidgets)
{
	sparkle_test::RecordingWidget root("Root");
	sparkle_test::RecordingWidget child("Child", &root);

	root.activate();
	child.activate();

	spk::MouseEventRecord event = makeMousePress();
	sparkle_test::sendMouseEvent(root, event);

	EXPECT_EQ(root.mouseEventCount, 1);
	EXPECT_EQ(child.mouseEventCount, 1);
}

// --- Focus types are independent ---

TEST_F(WidgetFocusTest, KeyboardAndMouseFocusAreIndependent)
{
	sparkle_test::RecordingWidget keyboardHolder("KeyboardHolder");
	sparkle_test::RecordingWidget mouseHolder("MouseHolder");

	keyboardHolder.takeFocus(spk::Widget::FocusType::Keyboard);
	mouseHolder.takeFocus(spk::Widget::FocusType::Mouse);

	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Keyboard), &keyboardHolder);
	EXPECT_EQ(spk::Widget::focusedWidget(spk::Widget::FocusType::Mouse), &mouseHolder);
	EXPECT_TRUE(keyboardHolder.hasFocus(spk::Widget::FocusType::Keyboard));
	EXPECT_FALSE(keyboardHolder.hasFocus(spk::Widget::FocusType::Mouse));
	EXPECT_TRUE(mouseHolder.hasFocus(spk::Widget::FocusType::Mouse));
	EXPECT_FALSE(mouseHolder.hasFocus(spk::Widget::FocusType::Keyboard));
}
