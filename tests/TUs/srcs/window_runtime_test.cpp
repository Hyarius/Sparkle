#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "window_test_utils.hpp"

TEST(WindowRuntimeTest, ConstructionActivatesRootWidgetAndExposesAccessors)
{
	auto bundle = sparkle_test::createRuntimeBundle(sparkle_test::defaultRect(), "Runtime");

	ASSERT_NE(bundle.runtime, nullptr);
	EXPECT_TRUE(bundle.runtime->rootWidget().isActivated());
	EXPECT_EQ(bundle.runtime->rootWidget().name(), ":/Runtime/RootWidget");
	EXPECT_EQ(bundle.runtime->window().title(), "Runtime");
	EXPECT_EQ(bundle.runtime->window().rect(), sparkle_test::defaultRect());
	EXPECT_EQ(bundle.runtime->mouse().position, spk::Vector2Int(0, 0));
	EXPECT_EQ(bundle.runtime->keyboard().glyph, U'\0');
}

TEST(WindowRuntimeTest, PollEventsDrivesWindowModulesAndWidgetTree)
{
	auto bundle = sparkle_test::createRuntimeBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.runtime->rootWidget());
	child.activate();

	const spk::Rect2D resizedRect(0, 0, 500, 300);
	bundle.frameBackend->queueFramePayload(spk::WindowResizedPayload{
		.rect = resizedRect});
	bundle.frameBackend->queueMousePayload(spk::MouseMovedPayload{
		.position = spk::Vector2Int(7, 9),
		.delta = spk::Vector2Int(2, 3)});
	bundle.frameBackend->queueKeyboardPayload(spk::KeyPressedPayload{
		.key = spk::Keyboard::C,
		.isRepeated = false});

	bundle.runtime->pollEvents();

	EXPECT_EQ(bundle.frameBackend->pumpEventsCount, 1);
	EXPECT_EQ(bundle.runtime->mouse().position, spk::Vector2Int(7, 9));
	EXPECT_EQ(bundle.runtime->mouse().deltaPosition, spk::Vector2Int(2, 3));
	EXPECT_EQ(bundle.runtime->keyboard()[spk::Keyboard::C], spk::InputState::Down);
	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->notifyResizeCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->lastResizeRect, resizedRect);
	EXPECT_EQ(child.frameEventCount, 1);
	EXPECT_EQ(child.mouseEventCount, 1);
	EXPECT_EQ(child.keyboardEventCount, 1);
}

TEST(WindowRuntimeTest, UpdateDelegatesToRootWidgetTree)
{
	auto bundle = sparkle_test::createRuntimeBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.runtime->rootWidget());
	child.activate();

	spk::UpdateTick tick;
	tick.mouse = &bundle.runtime->mouse();
	tick.keyboard = &bundle.runtime->keyboard();

	bundle.runtime->update(tick);

	EXPECT_EQ(child.updateCount, 1);
	EXPECT_EQ(child.lastTickMouse, &bundle.runtime->mouse());
	EXPECT_EQ(child.lastTickKeyboard, &bundle.runtime->keyboard());
}

TEST(WindowRuntimeTest, RenderMakesContextCurrentRendersTreeAndPresents)
{
	auto bundle = sparkle_test::createRuntimeBundle();
	sparkle_test::RecordingWidget child("Child", &bundle.runtime->rootWidget());
	child.activate();

	bundle.runtime->render();

	ASSERT_NE(bundle.renderBackend->createdContext, nullptr);
	EXPECT_EQ(bundle.renderBackend->createdContext->makeCurrentCount, 1);
	EXPECT_EQ(bundle.renderBackend->createdContext->presentCount, 1);
	EXPECT_EQ(child.renderCount, 1);
}

TEST(WindowRuntimeTest, RequestClosureDelegatesToManagedWindow)
{
	auto bundle = sparkle_test::createRuntimeBundle();

	bundle.runtime->requestClosure();

	ASSERT_NE(bundle.frameBackend->createdFrame, nullptr);
	EXPECT_EQ(bundle.frameBackend->createdFrame->requestClosureCount, 1);
}

TEST(WindowRuntimeTest, ClosureSubscribersAreTriggeredByValidatedCloseEvents)
{
	auto bundle = sparkle_test::createRuntimeBundle();
	int closureCount = 0;

	auto contract = bundle.runtime->subscribeToClosure([&](spk::WindowRuntime* p_runtime)
	{
		++closureCount;
		EXPECT_EQ(p_runtime, bundle.runtime.get());
	});

	bundle.frameBackend->queueFramePayload(spk::WindowCloseValidatedPayload{});
	bundle.runtime->pollEvents();

	EXPECT_EQ(closureCount, 1);
}

TEST(WindowRuntimeRegistryTest, CreateWindowRuntimeReturnsSharedRuntimeAndRejectsDuplicateIds)
{
	spk::WindowRuntimeRegistry registry;

	auto frameBackend = std::make_unique<sparkle_test::TestFrameBackend>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	std::shared_ptr<spk::WindowRuntime> runtime = registry.createWindowRuntime(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.frameBackend = std::move(frameBackend),
			.renderBackend = std::move(renderBackend)
		});

	ASSERT_NE(runtime, nullptr);
	EXPECT_EQ(runtime->window().title(), "Main");

	auto duplicateFrameBackend = std::make_unique<sparkle_test::TestFrameBackend>();
	auto duplicateRenderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();

	EXPECT_THROW(
		registry.createWindowRuntime(
			"main",
			spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "MainDuplicate",
				.frameBackend = std::move(duplicateFrameBackend),
				.renderBackend = std::move(duplicateRenderBackend)
			}),
		std::runtime_error);
}

TEST(WindowRuntimeRegistryTest, RequestWindowClosingDelegatesToStoredRuntime)
{
	spk::WindowRuntimeRegistry registry;
	auto frameBackend = std::make_unique<sparkle_test::TestFrameBackend>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* frameBackendPtr = frameBackend.get();

	std::shared_ptr<spk::WindowRuntime> runtime = registry.createWindowRuntime(
		"main",
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.frameBackend = std::move(frameBackend),
			.renderBackend = std::move(renderBackend)
		});

	registry.requestWindowClosing("main");

	ASSERT_NE(frameBackendPtr->createdFrame, nullptr);
	EXPECT_EQ(frameBackendPtr->createdFrame->requestClosureCount, 1);
}

TEST(WindowRuntimeRegistryTest, RequestWindowClosingThrowsForUnknownRuntime)
{
	spk::WindowRuntimeRegistry registry;

	EXPECT_THROW(registry.requestWindowClosing("missing"), std::runtime_error);
}
