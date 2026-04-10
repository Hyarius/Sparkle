#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "spk_window_registry.hpp"
#include "window_test_utils.hpp"

TEST(WindowRegistryTest, CreateWindowReturnsSharedWindowAndRejectsDuplicateIds)
{
	spk::WindowRegistry registry;

	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	std::shared_ptr<spk::Window> window = registry.createWindow(
		"main",
		platformRuntime,
		gpuPlatformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
		});

	ASSERT_NE(window, nullptr);
	EXPECT_EQ(window->host().title(), "Main");

	auto duplicatePlatformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto duplicateGPUPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();

	EXPECT_THROW(
		registry.createWindow(
			"main",
			duplicatePlatformRuntime,
			duplicateGPUPlatformRuntime,
			spk::Window::Configuration{
				.rect = sparkle_test::defaultRect(),
				.title = "MainDuplicate"
			}),
		std::runtime_error);
}

TEST(WindowRegistryTest, RequestWindowClosingDelegatesToStoredWindow)
{
	spk::WindowRegistry registry;
	auto platformRuntime = std::make_shared<sparkle_test::TestPlatformRuntime>();
	auto gpuPlatformRuntime = std::make_shared<sparkle_test::TestGPUPlatformRuntime>();
	auto* platformRuntimePtr = platformRuntime.get();

	std::shared_ptr<spk::Window> window = registry.createWindow(
		"main",
		platformRuntime,
		gpuPlatformRuntime,
		spk::Window::Configuration{
			.rect = sparkle_test::defaultRect(),
			.title = "Main"
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
