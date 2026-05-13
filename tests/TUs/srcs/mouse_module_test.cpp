#include <gtest/gtest.h>

#include <utility>

#include "spk_mouse_module.hpp"
#include "window_test_utils.hpp"

TEST(MouseModuleTest, MouseEventsUpdateInternalStateAndDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("MouseWidget");
	widget.activate();

	spk::MouseModule module;
	module.bind(&widget);

	spk::MouseEventRecord firstMove = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(36, 45),
		.delta = spk::Vector2Int(4, 5)}));
	spk::MouseEventRecord secondMove = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(40, 50),
		.delta = spk::Vector2Int(4, 5)}));
	spk::MouseEventRecord wheel = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseWheelScrolledRecord{
		.delta = spk::Vector2(0.0f, 1.5f)}));
	spk::MouseEventRecord press = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	spk::MouseEventRecord release = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{
		.button = spk::Mouse::Left}));

	module.pushEvent(std::move(firstMove));
	module.pushEvent(std::move(secondMove));
	module.pushEvent(std::move(wheel));
	module.pushEvent(std::move(press));
	module.pushEvent(std::move(release));

	module.processEvents();

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(40, 50));
	EXPECT_EQ(module.mouse().deltaPosition, spk::Vector2Int(4, 5));
	EXPECT_FLOAT_EQ(module.mouse().wheel, 1.5f);
	EXPECT_EQ(module.mouse()[spk::Mouse::Left], spk::InputState::Up);
	EXPECT_EQ(widget.mouseEventCount, 5);
}

TEST(MouseModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::MouseModule module;

	spk::MouseEventRecord event = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(9, 9),
		.delta = spk::Vector2Int(1, 1)}));

	EXPECT_NO_THROW(module.pushEvent(std::move(event)));
	EXPECT_NO_THROW(module.processEvents());

	EXPECT_EQ(module.mouse().position, spk::Vector2Int(0, 0));
}
