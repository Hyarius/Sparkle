#include "structures/widget/spk_widget_visual_test_helpers.hpp"

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"

namespace spk::test
{
	sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect)
	{
		sparkle_test::OpenGLTestContext context(p_captureRect);

		const auto expectedDir = expectedDirectory() / p_widgetName;
		const auto resultDir = resultDirectory() / p_widgetName;
		std::filesystem::create_directories(expectedDir);
		std::filesystem::create_directories(resultDir);

		const auto expectedPath = expectedDir / (p_variant + ".png");
		const auto actualPath = resultDir / (p_variant + "_actual.png");
		const auto diffPath = resultDir / (p_variant + "_diff.png");

		p_widget.setGeometry(p_captureRect.atOrigin());
		p_widget.activate();

		spk::RenderContext& renderContext = context.renderContext();
		glScissor(0, 0, static_cast<GLsizei>(p_captureRect.width()), static_cast<GLsizei>(p_captureRect.height()));
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
