#include "spk_widget_visual_test_helpers.hpp"

#include <exception>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
#include <Windows.h>
#include <GL/gl.h>
#endif

#include "opengl/spk_opengl_runtime.hpp"
#include "application/spk_platform_runtime.hpp"
#include "rendering/spk_render_snapshot_builder.hpp"
#include "winapi/spk_winapi_platform_runtime.hpp"

namespace spk::test
{
	namespace
	{
#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
		class OpenGLVisualTestContext
		{
		private:
			spk::WinAPI::PlatformRuntime _platformRuntime;
			spk::OpenGL::Runtime _gpuRuntime;
			std::unique_ptr<spk::IFrame> _frame;
			std::unique_ptr<spk::IRenderContext> _renderContext;

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

			spk::IRenderContext& renderContext()
			{
				_renderContext->makeCurrent();
				return *_renderContext;
			}

			spk::OpenGL::Runtime& gpuRuntime()
			{
				return _gpuRuntime;
			}
		};

		OpenGLVisualTestContext& visualContext()
		{
			static OpenGLVisualTestContext context;
			return context;
		}
#endif
	}

	std::filesystem::path visualTestResourcesRoot()
	{
		auto cursor = std::filesystem::current_path();
		for (int i = 0; i < 8; ++i)
		{
			auto candidate = cursor / "tests" / "TUs" / "resources";
			if (std::filesystem::exists(candidate) == true)
			{
				return candidate;
			}
			if (cursor.has_parent_path() == false)
			{
				break;
			}
			cursor = cursor.parent_path();
		}

		return std::filesystem::current_path() / "tests" / "TUs" / "resources";
	}

	std::filesystem::path expectedDirectory()
	{
		return visualTestResourcesRoot() / "expectedImages";
	}

	std::filesystem::path resultDirectory()
	{
		return visualTestResourcesRoot() / "imageResults";
	}

	sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect)
	{
#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)
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

		spk::IRenderContext& renderContext = context.renderContext();
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
#else
		(void)p_widget;
		(void)p_widgetName;
		(void)p_variant;
		(void)p_captureRect;
		ADD_FAILURE() << "OpenGL visual snapshot tests require Windows and SPARKLE_GPU_BACKEND_OPENGL";
		return {};
#endif
	}
}
