#include <gtest/gtest.h>

#include <type_traits>

#include "structures/system/event/spk_events.hpp"
#include "structures/system/time/spk_timestamp.hpp"
#include "type/spk_time_unit.hpp"

TEST(EventRecordTest, DefaultRecordStoresDefaultTimestamp)
{
	const spk::Timestamp before;
	spk::EventRecord record;
	const spk::Timestamp after;

	EXPECT_GE(record.timestamp, before);
	EXPECT_LE(record.timestamp, after);
}

TEST(EventRecordTest, ExplicitTimestampConstructorStoresTimestamp)
{
	const spk::Timestamp timestamp(123.0L, spk::TimeUnit::Millisecond);
	const spk::EventRecord record(timestamp);

	EXPECT_EQ(record.timestamp, timestamp);
}

TEST(EventViewTest, ViewProvidesConstAccessToRecord)
{
	spk::MouseMovedRecord record;
	record.position = spk::Vector2Int(1, 2);
	record.delta = spk::Vector2Int(3, 4);

	spk::MouseMovedEvent event(record, spk::Mouse{});

	EXPECT_EQ(event->position, spk::Vector2Int(1, 2));
	EXPECT_EQ(event->delta, spk::Vector2Int(3, 4));
	EXPECT_EQ((*event).position, spk::Vector2Int(1, 2));
	static_assert(std::is_const_v<std::remove_reference_t<decltype(event.data())>>);
}

TEST(EventViewTest, ConsumeUpdatesOnlyTheView)
{
	spk::WindowCloseRequestedRecord record;
	spk::WindowCloseRequestedEvent event(record);

	EXPECT_FALSE(event.isConsumed());

	event.consume();

	EXPECT_TRUE(event.isConsumed());
}

TEST(EventViewTest, ConsumeIsAvailableForEveryFrameEventInstantiation)
{
	spk::WindowCloseRequestedRecord close;
	spk::WindowDestroyedRecord destroyed;
	spk::WindowMovedRecord moved;
	spk::WindowResizedRecord resized;
	spk::WindowFocusGainedRecord focusGained;
	spk::WindowFocusLostRecord focusLost;
	spk::WindowShownRecord shown;
	spk::WindowHiddenRecord hidden;

	spk::WindowCloseRequestedEvent closeEvent(close);
	spk::WindowDestroyedEvent destroyedEvent(destroyed);
	spk::WindowMovedEvent movedEvent(moved);
	spk::WindowResizedEvent resizedEvent(resized);
	spk::WindowFocusGainedEvent focusGainedEvent(focusGained);
	spk::WindowFocusLostEvent focusLostEvent(focusLost);
	spk::WindowShownEvent shownEvent(shown);
	spk::WindowHiddenEvent hiddenEvent(hidden);

	closeEvent.consume();
	destroyedEvent.consume();
	movedEvent.consume();
	resizedEvent.consume();
	focusGainedEvent.consume();
	focusLostEvent.consume();
	shownEvent.consume();
	hiddenEvent.consume();

	EXPECT_TRUE(closeEvent.isConsumed());
	EXPECT_TRUE(destroyedEvent.isConsumed());
	EXPECT_TRUE(movedEvent.isConsumed());
	EXPECT_TRUE(resizedEvent.isConsumed());
	EXPECT_TRUE(focusGainedEvent.isConsumed());
	EXPECT_TRUE(focusLostEvent.isConsumed());
	EXPECT_TRUE(shownEvent.isConsumed());
	EXPECT_TRUE(hiddenEvent.isConsumed());
}

TEST(EventViewTest, ConsumeIsAvailableForEveryDeviceEventInstantiation)
{
	spk::Mouse mouse;
	spk::Keyboard keyboard;
	spk::MouseEnteredRecord entered;
	spk::MouseLeftRecord left;
	spk::MouseMovedRecord moved;
	spk::MouseWheelScrolledRecord wheel;
	spk::MouseButtonPressedRecord pressed;
	spk::MouseButtonReleasedRecord released;
	spk::MouseButtonDoubleClickedRecord doubleClicked;
	spk::KeyPressedRecord keyPressed;
	spk::KeyReleasedRecord keyReleased;
	spk::TextInputRecord text;

	spk::MouseEnteredWindowEvent enteredEvent(entered, mouse);
	spk::MouseLeftWindowEvent leftEvent(left, mouse);
	spk::MouseMovedEvent movedEvent(moved, mouse);
	spk::MouseWheelScrolledEvent wheelEvent(wheel, mouse);
	spk::MouseButtonPressedEvent pressedEvent(pressed, mouse);
	spk::MouseButtonReleasedEvent releasedEvent(released, mouse);
	spk::MouseButtonDoubleClickedEvent doubleClickedEvent(doubleClicked, mouse);
	spk::KeyPressedEvent keyPressedEvent(keyPressed, keyboard);
	spk::KeyReleasedEvent keyReleasedEvent(keyReleased, keyboard);
	spk::TextInputEvent textEvent(text, keyboard);

	enteredEvent.consume();
	leftEvent.consume();
	movedEvent.consume();
	wheelEvent.consume();
	pressedEvent.consume();
	releasedEvent.consume();
	doubleClickedEvent.consume();
	keyPressedEvent.consume();
	keyReleasedEvent.consume();
	textEvent.consume();

	EXPECT_TRUE(enteredEvent.isConsumed());
	EXPECT_TRUE(leftEvent.isConsumed());
	EXPECT_TRUE(movedEvent.isConsumed());
	EXPECT_TRUE(wheelEvent.isConsumed());
	EXPECT_TRUE(pressedEvent.isConsumed());
	EXPECT_TRUE(releasedEvent.isConsumed());
	EXPECT_TRUE(doubleClickedEvent.isConsumed());
	EXPECT_TRUE(keyPressedEvent.isConsumed());
	EXPECT_TRUE(keyReleasedEvent.isConsumed());
	EXPECT_TRUE(textEvent.isConsumed());
}

TEST(EventViewTest, TimestampReturnsRecordTimestamp)
{
	const spk::Timestamp timestamp(777.0L, spk::TimeUnit::Millisecond);
	spk::WindowShownRecord record;
	record.timestamp = timestamp;

	spk::WindowShownEvent event(record);

	EXPECT_EQ(event.timestamp(), timestamp);
}

TEST(EventViewTest, DifferentEventViewsAreNotConvertible)
{
	static_assert(std::is_constructible_v<
				  spk::EventView<spk::MouseMovedRecord>,
				  const spk::MouseMovedRecord&>);

	static_assert(!std::is_constructible_v<
				  spk::EventView<spk::MouseMovedRecord>,
				  spk::EventView<spk::KeyPressedRecord>&>);
}

TEST(DeviceEventViewTest, ProvidesAccessToRecordAndDevice)
{
	spk::MouseMovedRecord record;
	record.position = spk::Vector2Int(10, 20);
	record.delta = spk::Vector2Int(3, 4);

	spk::Mouse mouse;
	mouse.position = spk::Vector2Int(100, 200);
	mouse.deltaPosition = spk::Vector2Int(7, 8);
	mouse[spk::Mouse::Left] = spk::InputState::Down;

	spk::MouseMovedEvent event(record, mouse);

	EXPECT_EQ(event->position, spk::Vector2Int(10, 20));
	EXPECT_EQ(event->delta, spk::Vector2Int(3, 4));
	EXPECT_EQ(event.device().position, spk::Vector2Int(100, 200));
	EXPECT_EQ(event.device().deltaPosition, spk::Vector2Int(7, 8));
	EXPECT_EQ(event.device()[spk::Mouse::Left], spk::InputState::Down);
}

TEST(EventVariantTest, FrameMouseAndKeyboardFamiliesHoldOnlyTheirConcreteRecords)
{
	spk::WindowResizedRecord resizeRecord;
	resizeRecord.rect = spk::Rect2D(10, 20, 300, 400);
	spk::FrameEventRecord frameEvent = resizeRecord;

	spk::MouseWheelScrolledRecord wheelRecord;
	wheelRecord.delta = spk::Vector2(0.0f, 1.5f);
	spk::MouseEventRecord mouseEvent = wheelRecord;

	spk::TextInputRecord textRecord;
	textRecord.glyph = U'X';
	spk::KeyboardEventRecord keyboardEvent = textRecord;

	EXPECT_TRUE(spk::holds<spk::WindowResizedRecord>(frameEvent));
	EXPECT_FALSE(spk::holds<spk::WindowHiddenRecord>(frameEvent));
	EXPECT_TRUE(spk::holds<spk::MouseWheelScrolledRecord>(mouseEvent));
	EXPECT_TRUE(spk::holds<spk::TextInputRecord>(keyboardEvent));
}
