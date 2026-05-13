#include <gtest/gtest.h>

#include <utility>

#include "spk_keyboard_module.hpp"
#include "window_test_utils.hpp"

TEST(KeyboardModuleTest, KeyboardEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("KeyboardWidget");
	widget.activate();

	spk::KeyboardModule module;
	module.bind(&widget);

	spk::KeyboardEventRecord press = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
		.key = spk::Keyboard::Escape,
		.isRepeated = false}));
	spk::KeyboardEventRecord text = spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{
		.glyph = U'Q'}));
	spk::KeyboardEventRecord release = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyReleasedRecord{
		.key = spk::Keyboard::Escape}));

	module.pushEvent(std::move(press));
	module.pushEvent(std::move(text));
	module.pushEvent(std::move(release));

	module.processEvents();

	EXPECT_EQ(module.keyboard()[spk::Keyboard::Escape], spk::InputState::Up);
	EXPECT_EQ(module.keyboard().glyph, U'Q');
	EXPECT_EQ(widget.keyboardEventCount, 3);
}

TEST(KeyboardModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::KeyboardModule module;

	spk::KeyboardEventRecord event = spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{
		.key = spk::Keyboard::A,
		.isRepeated = false}));

	EXPECT_NO_THROW(module.pushEvent(std::move(event)));
	EXPECT_NO_THROW(module.processEvents());

	EXPECT_EQ(module.keyboard()[spk::Keyboard::A], spk::InputState::Up);
}
