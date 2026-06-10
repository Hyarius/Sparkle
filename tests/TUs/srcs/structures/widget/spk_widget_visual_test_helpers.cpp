#include "structures/widget/spk_widget_visual_test_helpers.hpp"

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/snapshot/spk_render_snapshot_builder.hpp"

namespace
{
	// Offscreen render target for visual captures. The hidden test window's back
	// buffer has undefined pixel ownership and keeps color/depth leftovers from
	// previous tests; a dedicated framebuffer guarantees a deterministic capture.
	class OffscreenCaptureTarget
	{
	private:
		GLuint _colorRenderbuffer = 0;
		GLuint _depthStencilRenderbuffer = 0;
		GLuint _framebuffer = 0;

	public:
		OffscreenCaptureTarget(GLsizei p_width, GLsizei p_height)
		{
			glGenRenderbuffers(1, &_colorRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, _colorRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, p_width, p_height);

			glGenRenderbuffers(1, &_depthStencilRenderbuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, _depthStencilRenderbuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, p_width, p_height);

			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glGenFramebuffers(1, &_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRenderbuffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _depthStencilRenderbuffer);
		}

		~OffscreenCaptureTarget()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &_framebuffer);
			glDeleteRenderbuffers(1, &_depthStencilRenderbuffer);
			glDeleteRenderbuffers(1, &_colorRenderbuffer);
		}

		[[nodiscard]] bool isComplete() const
		{
			return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
		}
	};

	[[nodiscard]] sparkle_test::ImageComparisonResult failedResult(const spk::Rect2D& p_captureRect)
	{
		return sparkle_test::ImageComparisonResult{
			.matches = false,
			.actualWidth = static_cast<int>(p_captureRect.width()),
			.actualHeight = static_cast<int>(p_captureRect.height()),
			.expectedWidth = 0,
			.expectedHeight = 0,
			.differentPixelCount = static_cast<std::size_t>(p_captureRect.width()) * static_cast<std::size_t>(p_captureRect.height())
		};
	}
}

namespace spk::test
{
	sparkle_test::ImageComparisonResult compareSnapshot(
		spk::Widget& p_widget,
		const std::string& p_widgetName,
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect)
	{
		sparkle_test::OpenGLTestContext context(p_captureRect);

		const auto expectedPath = expectedDirectory() / p_widgetName / (p_variant + ".png");
		const auto resultDir = resultDirectory() / p_widgetName;
		const auto actualPath = resultDir / (p_variant + "_actual.png");
		const auto diffPath = resultDir / (p_variant + "_diff.png");

		p_widget.setGeometry(p_captureRect.atOrigin());
		p_widget.activate();

		spk::RenderContext& renderContext = context.renderContext();

		const GLsizei width = static_cast<GLsizei>(p_captureRect.width());
		const GLsizei height = static_cast<GLsizei>(p_captureRect.height());

		{
			OffscreenCaptureTarget target(width, height);
			if (target.isComplete() == false)
			{
				ADD_FAILURE() << "Offscreen capture framebuffer is incomplete for " << p_widgetName << " (" << p_variant << ")";
				return failedResult(p_captureRect);
			}

			glViewport(0, 0, width, height);
			glScissor(0, 0, width, height);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(1.0);
			glClearStencil(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			spk::RenderSnapshotBuilder builder;
			p_widget.appendRenderUnits(builder);
			spk::RenderSnapshot snapshot = builder.build();
			snapshot.execute(renderContext);
			context.gpuRuntime().waitUntilWorkDone();

			std::filesystem::create_directories(resultDir);
			context.gpuRuntime().saveScreenshot(actualPath, p_captureRect.atOrigin());
		}

		// Expected images are validated by hand and committed outside of the TU:
		// a missing one is a failure, never something the test generates itself.
		if (std::filesystem::exists(expectedPath) == false)
		{
			ADD_FAILURE() << "No expected image for " << p_widgetName << " (" << p_variant << ")."
						  << " Visually validate the actual image, then copy it to approve.\n"
						  << "  actual:   " << actualPath.string() << "\n"
						  << "  expected: " << expectedPath.string();
			return failedResult(p_captureRect);
		}

		sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);
		if (result.matches)
		{
			std::filesystem::remove(actualPath);
			if (std::filesystem::is_empty(resultDir))
			{
				std::filesystem::remove(resultDir);
			}
		}
		else
		{
			ADD_FAILURE() << "Visual mismatch for " << p_widgetName << " (" << p_variant << ")"
						  << ". Diff: " << diffPath.string()
						  << ", actual: " << actualPath.string();
		}

		return result;
	}
}
