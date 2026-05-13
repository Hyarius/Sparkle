#include <gtest/gtest.h>

#include <utility>

#include "spk_frame_module.hpp"
#include "window_test_utils.hpp"

TEST(FrameModuleTest, FrameEventsDispatchToBoundWidget)
{
	sparkle_test::RecordingWidget widget("FrameWidget");
	widget.activate();

	spk::FrameModule module;
	module.bind(&widget);

	const spk::Rect2D resizedRect(0, 0, 1920, 1080);
	spk::FrameEventRecord event = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowResizedRecord{
		.rect = resizedRect}));
	module.pushEvent(std::move(event));
	module.processEvents();

	EXPECT_EQ(widget.geometry(), resizedRect.atOrigin());
	EXPECT_EQ(widget.frameEventCount, 1);
	ASSERT_EQ(widget.frameEventKinds.size(), 1u);
	EXPECT_EQ(widget.frameEventKinds[0], "WindowResized");
}

TEST(FrameModuleTest, PushEventIsSafeWhenUnbound)
{
	spk::FrameModule module;
	const spk::Rect2D resizedRect(0, 0, 1920, 1080);

	spk::FrameEventRecord event = spk::FrameEventRecord(spk::makeEventRecord(spk::WindowResizedRecord{
		.rect = resizedRect}));

	EXPECT_NO_THROW(module.pushEvent(std::move(event)));
	EXPECT_NO_THROW(module.processEvents());
}
