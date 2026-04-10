#include <gtest/gtest.h>

#include "window_test_utils.hpp"

TEST(IFrameTest, TestFrameTracksResizeTitleAndClosureRequests)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Initial");
	const spk::Rect2D resizedRect(1, 2, 640, 480);

	frame.resize(resizedRect);
	frame.setTitle("Updated");
	frame.requestClosure();

	EXPECT_EQ(frame.rect(), resizedRect);
	EXPECT_EQ(frame.title(), "Updated");
	EXPECT_EQ(frame.resizeCount, 1);
	EXPECT_EQ(frame.setTitleCount, 1);
	EXPECT_EQ(frame.requestClosureCount, 1);
	ASSERT_EQ(frame.resizeHistory.size(), 1u);
	EXPECT_EQ(frame.resizeHistory[0], resizedRect);
	ASSERT_EQ(frame.titleHistory.size(), 1u);
	EXPECT_EQ(frame.titleHistory[0], "Updated");
}

TEST(IFrameTest, FrameCanCreateRenderContextThroughBackend)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	sparkle_test::TestRenderContextBackend backend;

	std::unique_ptr<spk::IRenderContext> context = frame.createRenderContext(backend);

	ASSERT_NE(context, nullptr);
	ASSERT_NE(backend.createdContext, nullptr);
	EXPECT_EQ(frame.createRenderContextCount, 1);
	EXPECT_EQ(frame.lastRenderBackend, &backend);
	EXPECT_EQ(backend.createRenderContextCount, 1);
	EXPECT_EQ(backend.lastFrame, &frame);
}

TEST(IPlatformRuntimeTest, CreateFrameReceivesRequestedRectAndTitle)
{
	sparkle_test::TestPlatformRuntime runtime;
	const spk::Rect2D rect(5, 6, 200, 100);

	std::unique_ptr<spk::IFrame> frame = runtime.createFrame(rect, "Window");

	ASSERT_NE(frame, nullptr);
	ASSERT_NE(runtime.createdFrame, nullptr);
	EXPECT_EQ(runtime.createFrameCount, 1);
	EXPECT_EQ(runtime.lastCreateRect, rect);
	EXPECT_EQ(runtime.lastCreateTitle, "Window");
	EXPECT_EQ(runtime.createdFrame->rect(), rect);
	EXPECT_EQ(runtime.createdFrame->title(), "Window");
}

TEST(IPlatformRuntimeTest, PollEventsDispatchesQueuedMouseKeyboardAndFrameEvents)
{
	sparkle_test::TestPlatformRuntime runtime;
	std::unique_ptr<spk::IFrame> frame = runtime.createFrame(sparkle_test::defaultRect(), "Window");
	int mouseCount = 0;
	int keyboardCount = 0;
	int frameCount = 0;

	auto mouseContract = runtime.createdFrame->subscribeToMouseEvents([&](const spk::Event& p_event)
	{
		++mouseCount;
		EXPECT_TRUE(p_event.holds<spk::MouseMovedPayload>());
	});

	auto keyboardContract = runtime.createdFrame->subscribeToKeyboardEvents([&](const spk::Event& p_event)
	{
		++keyboardCount;
		EXPECT_TRUE(p_event.holds<spk::KeyPressedPayload>());
	});

	auto frameContract = runtime.createdFrame->subscribeToFrameEvents([&](const spk::Event& p_event)
	{
		++frameCount;
		EXPECT_TRUE(p_event.holds<spk::WindowShownPayload>());
	});

	runtime.queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(10, 20),
		.delta = spk::Vector2Int(1, 2)});
	runtime.queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::A,
		.isRepeated = false});
	runtime.queueFramePayload(spk::WindowShownPayload{});

	runtime.pollEvents();

	EXPECT_EQ(runtime.pollEventsCount, 1);
	EXPECT_EQ(mouseCount, 1);
	EXPECT_EQ(keyboardCount, 1);
	EXPECT_EQ(frameCount, 1);
}

TEST(IPlatformRuntimeTest, RuntimeCanBeConfiguredToReturnNullFrame)
{
	sparkle_test::TestPlatformRuntime runtime;
	runtime.returnNullFrame = true;

	std::unique_ptr<spk::IFrame> frame = runtime.createFrame(sparkle_test::defaultRect(), "NullFrame");

	EXPECT_EQ(frame, nullptr);
	EXPECT_EQ(runtime.createFrameCount, 1);
	EXPECT_EQ(runtime.createdFrame, nullptr);
}

TEST(IFrameTest, ResignedContractStopsReceivingEvents)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	int mouseCount = 0;

	auto contract = frame.subscribeToMouseEvents([&](const spk::Event&)
	{
		++mouseCount;
	});

	frame.emitMouseEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(1, 1),
		.delta = spk::Vector2Int(1, 1)}));
	ASSERT_EQ(mouseCount, 1);

	contract.resign();

	frame.emitMouseEvent(spk::Event(spk::MouseMovedPayload{
		.position = spk::Vector2Int(2, 2),
		.delta = spk::Vector2Int(2, 2)}));

	EXPECT_EQ(mouseCount, 1);
}

TEST(IFrameTest, MultipleSubscribersReceiveSameFrameEvent)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	int firstCount = 0;
	int secondCount = 0;

	auto firstContract = frame.subscribeToFrameEvents([&](const spk::Event&)
	{
		++firstCount;
	});
	auto secondContract = frame.subscribeToFrameEvents([&](const spk::Event&)
	{
		++secondCount;
	});

	frame.emitFrameEvent(spk::Event(spk::WindowHiddenPayload{}));

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
}
