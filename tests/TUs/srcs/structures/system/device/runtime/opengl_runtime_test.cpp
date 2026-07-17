#include <gtest/gtest.h>


#include <filesystem>
#include <memory>
#include <stdexcept>

#include <GL/glew.h>

#include <Windows.h>

#include "structures/system/device/runtime/spk_platform_runtime.hpp"
#include "structures/system/win32/spk_winapi_platform_runtime.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	void pumpWinApiMessages()
	{
		MSG message{};
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}

	[[nodiscard]] std::filesystem::path openglEdgeCaseResultDirectory()
	{
		std::filesystem::path result = std::filesystem::temp_directory_path() / "SparkleOpenGLEdgeCaseTests";
		std::filesystem::create_directories(result);
		return result;
	}
}

TEST(OpenGLRuntimeTest, CreateRenderContextRejectsNonWinApiFrames)
{
	spk::PlatformRuntime runtime;
	sparkle_test::TestFrame frame(spk::Rect2D(0, 0, 32, 32), "NonWinApi");

	EXPECT_THROW(runtime.createRenderContext(frame), std::runtime_error);
}

TEST(OpenGLRuntimeTest, SaveScreenshotRejectsEmptyCaptureRects)
{
	spk::PlatformRuntime runtime;
	const std::filesystem::path outputPath = openglEdgeCaseResultDirectory() / "empty.png";

	EXPECT_THROW(runtime.saveScreenshot(outputPath, spk::Rect2D(0, 0, 0, 1)), std::runtime_error);
	EXPECT_THROW(runtime.saveScreenshot(outputPath, spk::Rect2D(0, 0, 1, 0)), std::runtime_error);
	EXPECT_THROW(runtime.saveScreenshot(outputPath, spk::Rect2D(0, 0, 0, 0)), std::runtime_error);
}

TEST(OpenGLRuntimeTest, SaveScreenshotThrowsWhenPngCannotBeWritten)
{
	spk::PlatformRuntime platformRuntime;
	spk::PlatformRuntime gpuRuntime;
	std::unique_ptr<spk::IFrame> frame = platformRuntime.createFrame(spk::Rect2D(300, 300, 16, 16), "ScreenshotFailure");
	ASSERT_NE(frame, nullptr);

	std::unique_ptr<spk::RenderContext> renderContext = gpuRuntime.createRenderContext(*frame);
	ASSERT_NE(renderContext, nullptr);
	renderContext->makeCurrent();
	renderContext->notifyResize(spk::Rect2D(0, 0, 16, 16));

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	gpuRuntime.waitUntilWorkDone();

	EXPECT_THROW(gpuRuntime.saveScreenshot(openglEdgeCaseResultDirectory(), spk::Rect2D(0, 0, 16, 16)), std::runtime_error);

	frame->validateClosure();
	pumpWinApiMessages();
}

TEST(OpenGLRuntimeTest, SaveScreenshotUsesCurrentViewportWhenNoRectIsProvided)
{
	spk::PlatformRuntime platformRuntime;
	spk::PlatformRuntime gpuRuntime;
	std::unique_ptr<spk::IFrame> frame = platformRuntime.createFrame(spk::Rect2D(320, 320, 8, 6), "ViewportScreenshot");
	ASSERT_NE(frame, nullptr);

	std::unique_ptr<spk::RenderContext> renderContext = gpuRuntime.createRenderContext(*frame);
	ASSERT_NE(renderContext, nullptr);
	renderContext->makeCurrent();
	renderContext->notifyResize(spk::Rect2D(0, 0, 8, 6));

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	gpuRuntime.waitUntilWorkDone();

	const std::filesystem::path outputPath = openglEdgeCaseResultDirectory() / "viewport.png";
	std::filesystem::remove(outputPath);

	gpuRuntime.saveScreenshot(outputPath);

	EXPECT_TRUE(std::filesystem::exists(outputPath));
	EXPECT_GT(std::filesystem::file_size(outputPath), 0u);

	frame->validateClosure();
	pumpWinApiMessages();
}

