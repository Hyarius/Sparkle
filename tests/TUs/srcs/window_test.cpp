#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "window_test_utils.hpp"

TEST(WindowHostTest, ConstructionCreatesFrameAndRenderContextThroughPlatformRuntimeAndFrame)
{
	const spk::Rect2D rect(7, 8, 640, 360);
	auto bundle = sparkle_test::createWindowHostBundle(rect, "MainWindow");

	ASSERT_NE(bundle.windowHost, nullptr);
	ASSERT_NE(bundle.platformRuntime, nullptr);
	ASSERT_NE(bundle.renderBackend, nullptr);
	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);

	EXPECT_EQ(bundle.platformRuntime->createFrameCount, 1);
	EXPECT_EQ(bundle.platformRuntime->lastCreateRect, rect);
	EXPECT_EQ(bundle.platformRuntime->lastCreateTitle, "MainWindow");
	EXPECT_EQ(bundle.platformRuntime->createdFrame->createRenderContextCount, 1);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->lastRenderBackend, bundle.renderBackend);
	EXPECT_EQ(bundle.renderBackend->createRenderContextCount, 1);
	EXPECT_EQ(bundle.renderBackend->lastFrame, bundle.platformRuntime->createdFrame);
}

TEST(WindowHostTest, ConstructionWithoutExplicitRuntimeOrBackendThrows)
{
	EXPECT_THROW(
		{
			spk::WindowHost windowHost(spk::WindowHost::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NoBackends"
			});
		},
		std::runtime_error);
}

TEST(WindowHostTest, ConstructionRejectsNullFrameCreatedByPlatformRuntime)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	platformRuntime->returnNullFrame = true;

	EXPECT_THROW(
		{
			spk::WindowHost windowHost(spk::WindowHost::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NullFrame",
				.platformRuntime = std::move(platformRuntime),
				.renderBackend = std::move(renderBackend)
			});
		},
		std::runtime_error);
}

TEST(WindowHostTest, ConstructionRejectsNullRenderContextCreatedByBackend)
{
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	renderBackend->returnNullContext = true;

	EXPECT_THROW(
		{
			spk::WindowHost windowHost(spk::WindowHost::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NullContext",
				.platformRuntime = std::move(platformRuntime),
				.renderBackend = std::move(renderBackend)
			});
		},
		std::runtime_error);
}

TEST(WindowHostTest, FrameOperationsDelegateToCreatedFrame)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	const spk::Rect2D resizedRect(100, 200, 800, 600);

	bundle.windowHost->resize(resizedRect);
	bundle.windowHost->setTitle("Renamed");
	bundle.windowHost->requestClosure();

	ASSERT_NE(bundle.platformRuntime->createdFrame, nullptr);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->resizeCount, 1);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->setTitleCount, 1);
	EXPECT_EQ(bundle.platformRuntime->createdFrame->requestClosureCount, 1);
	EXPECT_EQ(bundle.windowHost->rect(), resizedRect);
	EXPECT_EQ(bundle.windowHost->title(), "Renamed");
}

TEST(WindowHostTest, RenderContextOperationsDelegateToCreatedContext)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	const spk::Rect2D resizedRect(2, 3, 400, 250);

	bundle.windowHost->makeCurrent();
	bundle.windowHost->present();
	bundle.windowHost->setVSync(true);
	bundle.windowHost->notifyFrameResized(resizedRect);

	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->presentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->setVSyncCount, 1);
	EXPECT_TRUE(bundle.renderBackend->createdContext->lastVSync);
	EXPECT_EQ(bundle.renderBackend->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->lastResizeRect, resizedRect);
}

TEST(WindowHostTest, PollEventsDelegatesToPlatformRuntimeAndForwardsSubscriptions)
{
	auto bundle = sparkle_test::createWindowHostBundle();
	int mouseCount = 0;
	int keyboardCount = 0;
	int frameCount = 0;

	auto mouseContract = bundle.windowHost->subscribeToMouseEvents([&](const spk::Event& p_event)
	{
		++mouseCount;
		EXPECT_TRUE(p_event.holds<spk::MouseMovedPayload>());
	});
	auto keyboardContract = bundle.windowHost->subscribeToKeyboardEvents([&](const spk::Event& p_event)
	{
		++keyboardCount;
		EXPECT_TRUE(p_event.holds<spk::KeyPressedPayload>());
	});
	auto frameContract = bundle.windowHost->subscribeToFrameEvents([&](const spk::Event& p_event)
	{
		++frameCount;
		EXPECT_TRUE(p_event.holds<spk::WindowResizedPayload>());
	});

	bundle.platformRuntime->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(2, 4),
		.delta = spk::Vector2Int(1, 1)});
	bundle.platformRuntime->queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::B,
		.isRepeated = true});
	bundle.platformRuntime->queueFramePayload(spk::WindowResizedPayload{
		.rect = spk::Rect2D(0, 0, 123, 456)});

	bundle.windowHost->pollEvents();

	EXPECT_EQ(bundle.platformRuntime->pollEventsCount, 1);
	EXPECT_EQ(mouseCount, 1);
	EXPECT_EQ(keyboardCount, 1);
	EXPECT_EQ(frameCount, 1);
}
