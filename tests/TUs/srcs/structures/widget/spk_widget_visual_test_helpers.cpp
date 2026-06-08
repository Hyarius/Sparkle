#include "structures/widget/spk_widget_visual_test_helpers.hpp"

#include <exception>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <Windows.h>
#include <GL/gl.h>

#include "structures/system/device/runtime/spk_opengl_runtime.hpp"
#include "structures/system/device/runtime/spk_platform_runtime.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"
#include "structures/system/win32/spk_winapi_platform_runtime.hpp"

namespace spk::test
{
	namespace
	{
		class OpenGLVisualTestContext
		{
		private:
			spk::PlatformRuntime _platformRuntime;
			spk::GPUPlatformRuntime _gpuRuntime;
			std::unique_ptr<spk::IFrame> _frame;
			std::unique_ptr<spk::RenderContext> _renderContext;

		public:
			OpenGLVisualTestContext() :
				_frame(_platformRuntime.createFrame(spk::Rect2D(0, 0, 1, 1), "Sparkle visual test context"))
			{
				_renderContext = _gpuRuntime.createRenderContext(*_frame);
				_renderContext->makeCurrent();
			}

			~OpenGLVisualTestContext()
			{
				if (_frame != nullptr)
				{
					_frame->validateClosure();
					_platformRuntime.pollEvents();
				}
			}

			void resize(const spk::Rect2D& p_rect)
			{
				_frame->resize(p_rect);
				_renderContext->notifyResize(p_rect.atOrigin());
			}

			spk::RenderContext& renderContext()
			{
				_renderContext->makeCurrent();
				return *_renderContext;
			}

			spk::GPUPlatformRuntime& gpuRuntime()
			{
				return _gpuRuntime;
			}
		};

		OpenGLVisualTestContext& visualContext()
		{
			static OpenGLVisualTestContext context;
			return context;
		}
	}

	sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect)
	{
		auto& context = visualContext();
		context.resize(p_captureRect);

		const auto expectedDir = expectedDirectory() / p_widgetName;
		const auto resultDir = resultDirectory() / p_widgetName;
		std::filesystem::create_directories(expectedDir);
		std::filesystem::create_directories(resultDir);

		const auto expectedPath = expectedDir / (p_variant + ".png");
		const auto actualPath = resultDir / (p_variant + "_actual.png");
		const auto diffPath = resultDir / (p_variant + "_diff.png");

		p_widget.setGeometry(p_captureRect.atOrigin());

		spk::RenderContext& renderContext = context.renderContext();
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		spk::RenderSnapshotBuilder builder;
		p_widget.appendRenderUnits(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(renderContext);
		context.gpuRuntime().waitUntilWorkDone();

		context.gpuRuntime().saveScreenshot(actualPath, p_captureRect.atOrigin());

		if (std::filesystem::exists(expectedPath) == false)
		{
			ADD_FAILURE() << "Missing expected image at " << expectedPath.string() << ". Saved actual output to " << actualPath.string();
			return sparkle_test::ImageComparisonResult{
				.matches = false,
				.actualWidth = static_cast<int>(p_captureRect.width()),
				.actualHeight = static_cast<int>(p_captureRect.height()),
				.expectedWidth = 0,
				.expectedHeight = 0,
				.differentPixelCount = static_cast<std::size_t>(p_captureRect.width()) * static_cast<std::size_t>(p_captureRect.height())
			};
		}

		sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);
		if (result.matches == false)
		{
			ADD_FAILURE() << "Visual mismatch for " << p_widgetName << " (" << p_variant << ")"
						  << ". Diff: " << diffPath.string()
						  << ", actual: " << actualPath.string();
		}

		return result;
	}
}
