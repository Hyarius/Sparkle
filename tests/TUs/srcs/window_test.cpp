#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "window_test_utils.hpp"

TEST(WindowTest, ConstructionCreatesFrameAndRenderContextThroughBackends)
{
	const spk::Rect2D rect(7, 8, 640, 360);
	auto bundle = sparkle_test::createWindowBundle(rect, "MainWindow");

	ASSERT_NE(bundle.window, nullptr);
	ASSERT_NE(bundle.frameBackend, nullptr);
	ASSERT_NE(bundle.renderBackend, nullptr);
	ASSERT_NE(bundle.frameBackend->createdFrame, nullptr);
	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);

	EXPECT_EQ(bundle.frameBackend->createFrameCount, 1);
	EXPECT_EQ(bundle.frameBackend->lastCreateRect, rect);
	EXPECT_EQ(bundle.frameBackend->lastCreateTitle, "MainWindow");
	EXPECT_EQ(bundle.renderBackend->createRenderContextCount, 1);
	EXPECT_EQ(bundle.renderBackend->lastFrame, bundle.frameBackend->createdFrame);
}

TEST(WindowTest, ConstructionWithoutExplicitBackendsThrows)
{
	EXPECT_THROW(
		{
			spk::Window window(spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NoBackends"
			});
		},
		std::runtime_error);
}

TEST(WindowTest, ConstructionRejectsNullFrameCreatedByBackend)
{
	auto frameBackend = std::make_unique<sparkle_test::TestFrameBackend>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	frameBackend->returnNullFrame = true;

	EXPECT_THROW(
		{
			spk::Window window(spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NullFrame",
				.frameBackend = std::move(frameBackend),
				.renderBackend = std::move(renderBackend)
			});
		},
		std::runtime_error);
}

TEST(WindowTest, ConstructionRejectsNullRenderContextCreatedByBackend)
{
	auto frameBackend = std::make_unique<sparkle_test::TestFrameBackend>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	renderBackend->returnNullContext = true;

	EXPECT_THROW(
		{
			spk::Window window(spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "NullContext",
				.frameBackend = std::move(frameBackend),
				.renderBackend = std::move(renderBackend)
			});
		},
		std::runtime_error);
}

TEST(WindowTest, FrameOperationsDelegateToCreatedFrame)
{
	auto bundle = sparkle_test::createWindowBundle();
	const spk::Rect2D resizedRect(100, 200, 800, 600);

	bundle.window->resize(resizedRect);
	bundle.window->setTitle("Renamed");
	bundle.window->requestClosure();

	ASSERT_NE(bundle.frameBackend->createdFrame, nullptr);
	EXPECT_EQ(bundle.frameBackend->createdFrame->resizeCount, 1);
	EXPECT_EQ(bundle.frameBackend->createdFrame->setTitleCount, 1);
	EXPECT_EQ(bundle.frameBackend->createdFrame->requestClosureCount, 1);
	EXPECT_EQ(bundle.window->rect(), resizedRect);
	EXPECT_EQ(bundle.window->title(), "Renamed");
}

TEST(WindowTest, RenderContextOperationsDelegateToCreatedContext)
{
	auto bundle = sparkle_test::createWindowBundle();
	const spk::Rect2D resizedRect(2, 3, 400, 250);

	bundle.window->makeCurrent();
	bundle.window->present();
	bundle.window->setVSync(true);
	bundle.window->notifyFrameResized(resizedRect);

	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->presentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->setVSyncCount, 1);
	EXPECT_TRUE(bundle.renderBackend->createdContext->lastVSync);
	EXPECT_EQ(bundle.renderBackend->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->lastResizeRect, resizedRect);
}

TEST(WindowTest, PumpEventsDelegatesToBackendAndForwardsSubscriptions)
{
	auto bundle = sparkle_test::createWindowBundle();
	int mouseCount = 0;
	int keyboardCount = 0;
	int frameCount = 0;

	auto mouseContract = bundle.window->subscribeToMouseEvents([&](const spk::Event& p_event)
	{
		++mouseCount;
		EXPECT_TRUE(p_event.holds<spk::MouseMovedPayload>());
	});
	auto keyboardContract = bundle.window->subscribeToKeyboardEvents([&](const spk::Event& p_event)
	{
		++keyboardCount;
		EXPECT_TRUE(p_event.holds<spk::KeyPressedPayload>());
	});
	auto frameContract = bundle.window->subscribeToFrameEvents([&](const spk::Event& p_event)
	{
		++frameCount;
		EXPECT_TRUE(p_event.holds<spk::WindowResizedPayload>());
	});

	bundle.frameBackend->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(2, 4),
		.delta = spk::Vector2Int(1, 1)});
	bundle.frameBackend->queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::B,
		.isRepeated = true});
	bundle.frameBackend->queueFramePayload(spk::WindowResizedPayload{
		.rect = spk::Rect2D(0, 0, 123, 456)});

	bundle.window->pumpEvents();

	EXPECT_EQ(bundle.frameBackend->pumpEventsCount, 1);
	EXPECT_EQ(mouseCount, 1);
	EXPECT_EQ(keyboardCount, 1);
	EXPECT_EQ(frameCount, 1);
}
