#include <gtest/gtest.h>

#include "window_test_utils.hpp"

namespace
{
	class NullSurfaceStateFrame : public spk::IFrame
	{
	public:
		NullSurfaceStateFrame() :
			spk::IFrame(nullptr)
		{
		}

		void resize(const spk::Rect2D&) override
		{
		}

		void setTitle(const std::string&) override
		{
		}

		void requestClosure() override
		{
		}

		void validateClosure() override
		{
		}

		[[nodiscard]] spk::Rect2D rect() const override
		{
			return {};
		}

		[[nodiscard]] std::string title() const override
		{
			return {};
		}
	};
}

TEST(IFrameTest, TestFrameTracksResizeTitleAndClosureRequests)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Initial");
	const spk::Rect2D resizedRect(1, 2, 640, 480);

	frame.resize(resizedRect);
	frame.setTitle("Updated");
	frame.requestClosure();
	frame.validateClosure();

	EXPECT_EQ(frame.rect(), resizedRect);
	EXPECT_EQ(frame.title(), "Updated");
	EXPECT_EQ(frame.resizeCount, 1);
	EXPECT_EQ(frame.setTitleCount, 1);
	EXPECT_EQ(frame.requestClosureCount, 1);
	EXPECT_EQ(frame.validateClosureCount, 1);
	ASSERT_EQ(frame.resizeHistory.size(), 1u);
	EXPECT_EQ(frame.resizeHistory[0], resizedRect);
	ASSERT_EQ(frame.titleHistory.size(), 1u);
	EXPECT_EQ(frame.titleHistory[0], "Updated");
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

	auto mouseContract = runtime.createdFrame->subscribeToMouseEvents([&](const spk::MouseEventRecord& p_event)
	{
		++mouseCount;
		EXPECT_TRUE(spk::holds<spk::MouseMovedRecord>(p_event));
	});

	auto keyboardContract = runtime.createdFrame->subscribeToKeyboardEvents([&](const spk::KeyboardEventRecord& p_event)
	{
		++keyboardCount;
		EXPECT_TRUE(spk::holds<spk::KeyPressedRecord>(p_event));
	});

	auto frameContract = runtime.createdFrame->subscribeToFrameEvents([&](const spk::FrameEventRecord& p_event)
	{
		++frameCount;
		EXPECT_TRUE(spk::holds<spk::WindowShownRecord>(p_event));
	});

	runtime.queueMouseEvent(spk::MouseMovedRecord{
		.position = spk::Vector2Int(10, 20),
		.delta = spk::Vector2Int(1, 2)});
	runtime.queueKeyboardEvent(spk::KeyPressedRecord{
		.key = spk::Keyboard::A,
		.isRepeated = false});
	runtime.queueFrameEvent(spk::WindowShownRecord{});

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

	auto contract = frame.subscribeToMouseEvents([&](const spk::MouseEventRecord&)
	{
		++mouseCount;
	});

	frame.emitMouseEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(1, 1),
		.delta = spk::Vector2Int(1, 1)})));
	ASSERT_EQ(mouseCount, 1);

	contract.resign();

	frame.emitMouseEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
		.position = spk::Vector2Int(2, 2),
		.delta = spk::Vector2Int(2, 2)})));

	EXPECT_EQ(mouseCount, 1);
}

TEST(IFrameTest, MultipleSubscribersReceiveSameFrameEvent)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	int firstCount = 0;
	int secondCount = 0;

	auto firstContract = frame.subscribeToFrameEvents([&](const spk::FrameEventRecord&)
	{
		++firstCount;
	});
	auto secondContract = frame.subscribeToFrameEvents([&](const spk::FrameEventRecord&)
	{
		++secondCount;
	});

	frame.emitFrameEvent(spk::FrameEventRecord(spk::makeEventRecord(spk::WindowHiddenRecord{})));

	EXPECT_EQ(firstCount, 1);
	EXPECT_EQ(secondCount, 1);
}

TEST(IFrameTest, DestroyedFrameEventInvalidatesSurfaceStateBeforeSubscribersAreNotified)
{
	sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
	std::shared_ptr<spk::ISurfaceState> surfaceState = frame.surfaceState();
	bool surfaceWasInvalidInsideCallback = false;

	ASSERT_NE(surfaceState, nullptr);
	EXPECT_TRUE(surfaceState->isValid());

	auto contract = frame.subscribeToFrameEvents([&](const spk::FrameEventRecord& p_event)
	{
		if (spk::holds<spk::WindowDestroyedRecord>(p_event))
		{
			surfaceWasInvalidInsideCallback = (surfaceState->isValid() == false);
		}
	});

	frame.emitFrameEvent(spk::FrameEventRecord(spk::makeEventRecord(spk::WindowDestroyedRecord{})));

	EXPECT_TRUE(surfaceWasInvalidInsideCallback);
	EXPECT_FALSE(surfaceState->isValid());
}

TEST(IFrameTest, ConstructingFrameWithoutSurfaceStateThrows)
{
	EXPECT_THROW(NullSurfaceStateFrame(), std::invalid_argument);
}

TEST(IFrameTest, DestroyingFrameInvalidatesItsSurfaceState)
{
	std::shared_ptr<spk::ISurfaceState> surfaceState;

	{
		sparkle_test::TestFrame frame(sparkle_test::defaultRect(), "Frame");
		surfaceState = frame.surfaceState();

		ASSERT_NE(surfaceState, nullptr);
		EXPECT_TRUE(surfaceState->isValid());
	}

	ASSERT_NE(surfaceState, nullptr);
	EXPECT_FALSE(surfaceState->isValid());
}
