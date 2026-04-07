#include <gtest/gtest.h>

#include <type_traits>

#include "spk_duration.hpp"
#include "spk_events.hpp"
#include "spk_timestamp.hpp"
#include "spk_time_unit.hpp"

TEST(EventMetadata_ConstructionTest, DefaultMetadataStartsUnconsumed)
{
	spk::EventMetadata metadata;

	EXPECT_FALSE(metadata.isConsumed);
}

TEST(EventMetadata_ConstructionTest, ExplicitTimestampConstructorStoresTimestamp)
{
	const spk::Timestamp timestamp(123.0L, spk::TimeUnit::Millisecond);
	const spk::EventMetadata metadata(timestamp);

	EXPECT_FALSE(metadata.isConsumed);
	EXPECT_EQ(metadata.timestamp, timestamp);
}

TEST(Event_ConstructionTest, PayloadOnlyConstructorBuildsMouseMovedEvent)
{
	spk::Event event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(10, 20),
		.delta = spk::Vector2Int(3, 4)});

	EXPECT_TRUE(event.holds<spk::MouseMovedPayload>());
	EXPECT_FALSE(event.metadata.isConsumed);

	const spk::MouseMovedPayload *payload = event.getIf<spk::MouseMovedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->position.x, 10);
	EXPECT_EQ(payload->position.y, 20);
	EXPECT_EQ(payload->delta.x, 3);
	EXPECT_EQ(payload->delta.y, 4);
}

TEST(Event_ConstructionTest, TimestampAndPayloadConstructorStoresBoth)
{
	const spk::Timestamp timestamp(2.0L, spk::TimeUnit::Second);

	spk::Event event(
		timestamp,
		spk::KeyPressedPayload{
			.key = spk::Keyboard::A,
			.isRepeated = true});

	EXPECT_TRUE(event.holds<spk::KeyPressedPayload>());
	EXPECT_EQ(event.metadata.timestamp, timestamp);
	EXPECT_FALSE(event.metadata.isConsumed);

	const spk::KeyPressedPayload *payload = event.getIf<spk::KeyPressedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->key, spk::Keyboard::A);
	EXPECT_TRUE(payload->isRepeated);
}

TEST(Event_ConstructionTest, MetadataAndPayloadConstructorStoresBoth)
{
	const spk::Timestamp timestamp(500.0L, spk::TimeUnit::Millisecond);
	const spk::EventMetadata metadata(timestamp);

	spk::Event event(
		metadata,
		spk::TextInputPayload{
			.glyph = U'K'});

	EXPECT_TRUE(event.holds<spk::TextInputPayload>());
	EXPECT_EQ(event.metadata.timestamp, timestamp);
	EXPECT_FALSE(event.metadata.isConsumed);

	const spk::TextInputPayload *payload = event.getIf<spk::TextInputPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->glyph, U'K');
}

TEST(Event_HoldsTest, HoldsReturnsFalseForWrongPayloadType)
{
	spk::Event event(spk::WindowShownPayload{});

	EXPECT_TRUE(event.holds<spk::WindowShownPayload>());
	EXPECT_FALSE(event.holds<spk::WindowHiddenPayload>());
	EXPECT_FALSE(event.holds<spk::MouseMovedPayload>());
}

TEST(Event_GetIfTest, GetIfReturnsNullptrForWrongType)
{
	spk::Event event(spk::WindowShownPayload{});

	EXPECT_NE(event.getIf<spk::WindowShownPayload>(), nullptr);
	EXPECT_EQ(event.getIf<spk::WindowHiddenPayload>(), nullptr);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>(), nullptr);
}

TEST(Event_ViewTest, ViewProvidesMutableAccessToPayload)
{
	spk::Event event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(1, 2),
		.delta = spk::Vector2Int(3, 4)});

	auto view = event.view<spk::MouseMovedPayload>();

	view.payload.position = spk::Vector2Int(10, 20);
	view.payload.delta = spk::Vector2Int(30, 40);

	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->position.x, 10);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->position.y, 20);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->delta.x, 30);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->delta.y, 40);
}

TEST(Event_ViewTest, ViewConsumeUpdatesMetadata)
{
	spk::Event event(spk::WindowCloseRequestedPayload{});

	auto view = event.view<spk::WindowCloseRequestedPayload>();

	EXPECT_FALSE(view.isConsumed());

	view.consume();

	EXPECT_TRUE(view.isConsumed());
	EXPECT_TRUE(event.metadata.isConsumed);
}

TEST(Event_ViewTest, ViewTimestampReturnsEventTimestamp)
{
	const spk::Timestamp timestamp(777.0L, spk::TimeUnit::Millisecond);

	spk::Event event(
		timestamp,
		spk::WindowShownPayload{});

	auto view = event.view<spk::WindowShownPayload>();

	EXPECT_EQ(view.timestamp(), timestamp);
}

TEST(Event_TypeAliasTest, WindowResizedEventAliasMatchesExpectedViewType)
{
	static_assert(std::is_same_v<
				  spk::WindowResizedEvent,
				  spk::Event::View<spk::WindowResizedPayload>>);
}

TEST(Event_TypeAliasTest, UpdateEventAliasMatchesExpectedViewType)
{
	static_assert(std::is_same_v<
				  spk::UpdateEvent,
				  spk::Event::View<spk::UpdatePayload>>);
}

TEST(Event_TypeAliasTest, MouseMovedEventAliasMatchesExpectedDeviceViewType)
{
	static_assert(std::is_same_v<
				  spk::MouseMovedEvent,
				  spk::Event::DeviceView<spk::MouseMovedPayload, spk::Mouse>>);
}

TEST(Event_TypeAliasTest, KeyPressedEventAliasMatchesExpectedDeviceViewType)
{
	static_assert(std::is_same_v<
				  spk::KeyPressedEvent,
				  spk::Event::DeviceView<spk::KeyPressedPayload, spk::Keyboard>>);
}

TEST(Event_WindowPayloadTest, WindowMovedPayloadStoresPosition)
{
	spk::Event event(spk::WindowMovedPayload{
		.position = spk::Vector2Int(12, 34)});

	const spk::WindowMovedPayload *payload = event.getIf<spk::WindowMovedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->position.x, 12);
	EXPECT_EQ(payload->position.y, 34);
}

TEST(Event_WindowPayloadTest, WindowResizedPayloadStoresRect)
{
	spk::Event event(spk::WindowResizedPayload{
		.rect = spk::Rect2D(10, 20, 300, 400)});

	const spk::WindowResizedPayload *payload = event.getIf<spk::WindowResizedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->rect.anchor.x, 10);
	EXPECT_EQ(payload->rect.anchor.y, 20);
	EXPECT_EQ(payload->rect.size.x, 300u);
	EXPECT_EQ(payload->rect.size.y, 400u);
}

TEST(Event_UpdatePayloadTest, UpdatePayloadStoresDeltaTime)
{
	spk::Event event(spk::UpdatePayload{
		.deltaTime = spk::Duration(16.0L, spk::TimeUnit::Millisecond)});

	const spk::UpdatePayload *payload = event.getIf<spk::UpdatePayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->deltaTime.milliseconds(), 16LL);
	EXPECT_EQ(payload->deltaTime.nanoseconds(), 16000000LL);
}

TEST(Event_UpdatePayloadTest, UpdatePayloadStoresDevices)
{
	spk::Mouse mouse;
	spk::Keyboard keyboard;

	spk::Event event(spk::UpdatePayload{
		.mouse = &mouse, .keyboard = &keyboard});

	const spk::UpdatePayload *payload = event.getIf<spk::UpdatePayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->mouse, &mouse);
	EXPECT_EQ(payload->keyboard, &keyboard);
}

TEST(Event_MousePayloadTest, MouseEnteredPayloadStoresPosition)
{
	spk::Event event(spk::MouseEnteredPayload{
		.position = spk::Vector2Int(50, 60)});

	const spk::MouseEnteredPayload *payload = event.getIf<spk::MouseEnteredPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->position.x, 50);
	EXPECT_EQ(payload->position.y, 60);
}

TEST(Event_MousePayloadTest, MouseWheelScrolledPayloadStoresDelta)
{
	spk::Event event(spk::MouseWheelScrolledPayload{
		.delta = spk::Vector2(0.0f, 1.5f)});

	const spk::MouseWheelScrolledPayload *payload = event.getIf<spk::MouseWheelScrolledPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_FLOAT_EQ(payload->delta.x, 0.0f);
	EXPECT_FLOAT_EQ(payload->delta.y, 1.5f);
}

TEST(Event_MousePayloadTest, MouseButtonPressedPayloadStoresButton)
{
	spk::Event event(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Right});

	const spk::MouseButtonPressedPayload *payload = event.getIf<spk::MouseButtonPressedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->button, spk::Mouse::Right);
}

TEST(Event_KeyboardPayloadTest, KeyPressedPayloadStoresKeyAndRepeatFlag)
{
	spk::Event event(spk::KeyPressedPayload{
		.key = spk::Keyboard::Escape,
		.isRepeated = true});

	const spk::KeyPressedPayload *payload = event.getIf<spk::KeyPressedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->key, spk::Keyboard::Escape);
	EXPECT_TRUE(payload->isRepeated);
}

TEST(Event_KeyboardPayloadTest, KeyReleasedPayloadStoresKey)
{
	spk::Event event(spk::KeyReleasedPayload{
		.key = spk::Keyboard::Return});

	const spk::KeyReleasedPayload *payload = event.getIf<spk::KeyReleasedPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->key, spk::Keyboard::Return);
}

TEST(Event_KeyboardPayloadTest, TextInputPayloadStoresGlyph)
{
	spk::Event event(spk::TextInputPayload{
		.glyph = U'X'});

	const spk::TextInputPayload *payload = event.getIf<spk::TextInputPayload>();
	ASSERT_NE(payload, nullptr);

	EXPECT_EQ(payload->glyph, U'X');
}
TEST(Event_DeviceViewTest, MouseDeviceViewProvidesAccessToPayloadAndDevice)
{
	spk::Event event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(10, 20),
		.delta = spk::Vector2Int(3, 4)});

	spk::Mouse mouse;
	mouse.position = spk::Vector2Int(100, 200);
	mouse.deltaPosition = spk::Vector2Int(7, 8);
	mouse[spk::Mouse::Left] = spk::InputState::Down;

	spk::Event::DeviceView<spk::MouseMovedPayload, spk::Mouse> deviceView{
		{event.metadata, *event.getIf<spk::MouseMovedPayload>()},
		mouse};

	EXPECT_EQ(deviceView.payload.position.x, 10);
	EXPECT_EQ(deviceView.payload.position.y, 20);
	EXPECT_EQ(deviceView.payload.delta.x, 3);
	EXPECT_EQ(deviceView.payload.delta.y, 4);

	EXPECT_EQ(deviceView.device.position.x, 100);
	EXPECT_EQ(deviceView.device.position.y, 200);
	EXPECT_EQ(deviceView.device.deltaPosition.x, 7);
	EXPECT_EQ(deviceView.device.deltaPosition.y, 8);
	EXPECT_EQ(deviceView.device[spk::Mouse::Left], spk::InputState::Down);
}

TEST(Event_DeviceViewTest, KeyboardDeviceViewProvidesAccessToPayloadAndDevice)
{
	spk::Event event(spk::KeyPressedPayload{
		.key = spk::Keyboard::A,
		.isRepeated = true});

	spk::Keyboard keyboard;
	keyboard[spk::Keyboard::A] = spk::InputState::Down;
	keyboard[spk::Keyboard::LeftShift] = spk::InputState::Down;
	keyboard.glyph = U'Z';

	spk::Event::DeviceView<spk::KeyPressedPayload, spk::Keyboard> deviceView{
		{event.metadata, *event.getIf<spk::KeyPressedPayload>()},
		keyboard};

	EXPECT_EQ(deviceView.payload.key, spk::Keyboard::A);
	EXPECT_TRUE(deviceView.payload.isRepeated);

	EXPECT_EQ(deviceView.device[spk::Keyboard::A], spk::InputState::Down);
	EXPECT_EQ(deviceView.device[spk::Keyboard::LeftShift], spk::InputState::Down);
	EXPECT_EQ(deviceView.device.glyph, U'Z');
}

TEST(Event_DeviceViewTest, DeviceViewConsumeUpdatesUnderlyingMetadata)
{
	spk::Event event(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Left});

	spk::Mouse mouse;

	spk::Event::DeviceView<spk::MouseButtonPressedPayload, spk::Mouse> deviceView{
		{event.metadata, *event.getIf<spk::MouseButtonPressedPayload>()},
		mouse};

	EXPECT_FALSE(deviceView.isConsumed());
	EXPECT_FALSE(event.metadata.isConsumed);

	deviceView.consume();

	EXPECT_TRUE(deviceView.isConsumed());
	EXPECT_TRUE(event.metadata.isConsumed);
}

TEST(Event_DeviceViewTest, DeviceViewTimestampReturnsUnderlyingTimestamp)
{
	const spk::Timestamp timestamp(42.0L, spk::TimeUnit::Millisecond);

	spk::Event event(
		timestamp,
		spk::KeyReleasedPayload{
			.key = spk::Keyboard::Escape});

	spk::Keyboard keyboard;

	spk::Event::DeviceView<spk::KeyReleasedPayload, spk::Keyboard> deviceView{
		{event.metadata, *event.getIf<spk::KeyReleasedPayload>()},
		keyboard};

	EXPECT_EQ(deviceView.timestamp(), timestamp);
}

TEST(Event_DeviceViewTest, DeviceViewAllowsPayloadMutationThroughInheritedView)
{
	spk::Event event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(1, 2),
		.delta = spk::Vector2Int(3, 4)});

	spk::Mouse mouse;

	spk::Event::DeviceView<spk::MouseMovedPayload, spk::Mouse> deviceView{
		{event.metadata, *event.getIf<spk::MouseMovedPayload>()},
		mouse};

	deviceView.payload.position = spk::Vector2Int(10, 20);
	deviceView.payload.delta = spk::Vector2Int(30, 40);

	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->position.x, 10);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->position.y, 20);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->delta.x, 30);
	EXPECT_EQ(event.getIf<spk::MouseMovedPayload>()->delta.y, 40);
}

TEST(Event_DeviceViewTest, MouseButtonPressedEventAliasCanBeConstructedFromDeviceView)
{
	spk::Event event(spk::MouseButtonPressedPayload{
		.button = spk::Mouse::Right});

	spk::Mouse mouse;
	mouse[spk::Mouse::Right] = spk::InputState::Down;

	spk::MouseButtonPressedEvent deviceView{
		{event.metadata, *event.getIf<spk::MouseButtonPressedPayload>()},
		mouse};

	EXPECT_EQ(deviceView.payload.button, spk::Mouse::Right);
	EXPECT_EQ(deviceView.device[spk::Mouse::Right], spk::InputState::Down);
}

TEST(Event_DeviceViewTest, KeyPressedEventAliasCanBeConstructedFromDeviceView)
{
	spk::Event event(spk::KeyPressedPayload{
		.key = spk::Keyboard::Return,
		.isRepeated = false});

	spk::Keyboard keyboard;
	keyboard[spk::Keyboard::Return] = spk::InputState::Down;

	spk::KeyPressedEvent deviceView{
		{event.metadata, *event.getIf<spk::KeyPressedPayload>()},
		keyboard};

	EXPECT_EQ(deviceView.payload.key, spk::Keyboard::Return);
	EXPECT_FALSE(deviceView.payload.isRepeated);
	EXPECT_EQ(deviceView.device[spk::Keyboard::Return], spk::InputState::Down);
}