#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "spk_window_registry.hpp"
#include "window_test_utils.hpp"

TEST(WindowRegistryTest, CreateWindowReturnsSharedWindowAndRejectsDuplicateIds)
{
	spk::WindowRegistry registry;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	std::shared_ptr<spk::Window> window = registry.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

	ASSERT_NE(window, nullptr);
	EXPECT_EQ(window->host().title(), "Main");

	auto duplicatePlatformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto duplicateRenderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();

	EXPECT_THROW(
		registry.createWindow(
			"main",
			spk::WindowHost::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "MainDuplicate",
				.platformRuntime = std::move(duplicatePlatformRuntime),
				.renderBackend = std::move(duplicateRenderBackend)
			}),
		std::runtime_error);
}

TEST(WindowRegistryTest, RequestWindowClosingDelegatesToStoredWindow)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto renderBackend = std::make_unique<sparkle_test::TestRenderContextBackend>();
	auto* platformRuntimePtr = platformRuntime.get();

	std::shared_ptr<spk::Window> window = registry.createWindow(
		"main",
		spk::WindowHost::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main",
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

	registry.requestWindowClosing("main");

	ASSERT_NE(platformRuntimePtr->createdFrame, nullptr);
	EXPECT_EQ(platformRuntimePtr->createdFrame->requestClosureCount, 1);
}

TEST(WindowRegistryTest, RequestWindowClosingThrowsForUnknownWindow)
{
	spk::WindowRegistry registry;

	EXPECT_THROW(registry.requestWindowClosing("missing"), std::runtime_error);
}
