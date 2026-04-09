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

TEST(IFrameBackendTest, CreateFrameReceivesRequestedRectAndTitle)
{
	sparkle_test::TestFrameBackend backend;
	const spk::Rect2D rect(5, 6, 200, 100);

	std::unique_ptr<spk::IFrame> frame = backend.createFrame(rect, "Window");

	ASSERT_NE(frame, nullptr);
	ASSERT_NE(backend.createdFrame, nullptr);
	EXPECT_EQ(backend.createFrameCount, 1);
	EXPECT_EQ(backend.lastCreateRect, rect);
	EXPECT_EQ(backend.lastCreateTitle, "Window");
	EXPECT_EQ(backend.createdFrame->rect(), rect);
	EXPECT_EQ(backend.createdFrame->title(), "Window");
}

TEST(IFrameBackendTest, PumpEventsDispatchesQueuedMouseKeyboardAndFrameEvents)
{
	sparkle_test::TestFrameBackend backend;
	int mouseCount = 0;
	int keyboardCount = 0;
	int frameCount = 0;

	auto mouseContract = backend.subscribeToMouseEvents([&](const spk::Event& p_event)
	{
		++mouseCount;
		EXPECT_TRUE(p_event.holds<spk::MouseMovedPayload>());
	});

	auto keyboardContract = backend.subscribeToKeyboardEvents([&](const spk::Event& p_event)
	{
		++keyboardCount;
		EXPECT_TRUE(p_event.holds<spk::KeyPressedPayload>());
	});

	auto frameContract = backend.subscribeToFrameEvents([&](const spk::Event& p_event)
	{
		++frameCount;
		EXPECT_TRUE(p_event.holds<spk::WindowShownPayload>());
	});

	backend.queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(10, 20),
		.delta = spk::Vector2Int(1, 2)});
	backend.queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::A,
		.isRepeated = false});
	backend.queueFramePayload(spk::WindowShownPayload{});

	backend.pumpEvents();

	EXPECT_EQ(backend.pumpEventsCount, 1);
	EXPECT_EQ(mouseCount, 1);
	EXPECT_EQ(keyboardCount, 1);
	EXPECT_EQ(frameCount, 1);
}

TEST(IFrameBackendTest, ResignedContractStopsReceivingEvents)
{
	sparkle_test::TestFrameBackend backend;
	int mouseCount = 0;

	auto contract = backend.subscribeToMouseEvents([&](const spk::Event&)
	{
		++mouseCount;
	});

	backend.emitMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(1, 1),
		.delta = spk::Vector2Int(1, 1)});
	ASSERT_EQ(mouseCount, 1);

	contract.resign();

	backend.emitMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(2, 2),
		.delta = spk::Vector2Int(2, 2)});

	EXPECT_EQ(mouseCount, 1);
}

TEST(IFrameBackendTest, MultipleSubscribersReceiveSameFrameEvent)
{
	sparkle_test::TestFrameBackend backend;
	int firstCount = 0;
	int secondCount = 0;

	auto firstContract = backend.subscribeToFrameEvents([&](const spk::Event&)
	{
		++firstCount;
	});
	auto secondContract = backend.subscribeToFrameEvents([&](const spk::Event&)
	{
		++secondCount;
	});

	backend.emitFramePayload(spk::WindowHiddenPayload{});

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
}
