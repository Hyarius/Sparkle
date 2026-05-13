#include <gtest/gtest.h>

#include <type_traits>

#include "spk_events.hpp"
#include "spk_timestamp.hpp"
#include "spk_time_unit.hpp"

TEST(EventRecordTest, DefaultRecordStoresDefaultTimestampAndZeroSequence)
{
	spk::EventRecord record;

	EXPECT_EQ(record.sequence, 0u);
}

TEST(EventRecordTest, ExplicitTimestampConstructorStoresTimestamp)
{
	const spk::Timestamp timestamp(123.0L, spk::TimeUnit::Millisecond);
	const spk::EventRecord record(timestamp);

	EXPECT_EQ(record.timestamp, timestamp);
	EXPECT_EQ(record.sequence, 0u);
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

TEST(EventVariantTest, SequenceCanBeAssignedToRecordsThroughFamilyVariant)
{
	spk::MouseMovedRecord record;
	record.position = spk::Vector2Int(1, 2);
	record.delta = spk::Vector2Int(3, 4);
	spk::MouseEventRecord event = record;

	spk::setEventSequence(event, 42);

	EXPECT_EQ(spk::eventSequence(event), 42u);
}
