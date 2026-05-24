#include <gtest/gtest.h>


#include <array>
#include <cstddef>
#include <filesystem>

#include <GL/glew.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"
#include "render_command_test_utils.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"
#include "rendering/render_command/spk_color_rectangle_render_command.hpp"
#include "rendering/spk_render_unit_builder.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

TEST(ColorRectangleRenderCommandTest, DrawsFullScreenRect)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(
		spk::Rect2D(0, 0, width, height),
		spk::Color(1.0f, 0.0f, 0.0f),
		0.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/full_actual"),
		sparkle_test::renderCommandExpectedPath("ColorRectangleRenderCommand/full_expected"),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/full_diff"));
}

TEST(ColorRectangleRenderCommandTest, DrawsPartialRect)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(
		spk::Rect2D(0, 0, width / 2, height),
		spk::Color(0.0f, 1.0f, 0.0f),
		0.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/partial_actual"),
		sparkle_test::renderCommandExpectedPath("ColorRectangleRenderCommand/partial_expected"),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/partial_diff"));
}

TEST(ColorRectangleRenderCommandTest, ConstructsWithoutValidViewport)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 32, 32));
	glViewport(0, 0, 0, 0);

	EXPECT_NO_THROW({
		spk::ColorRectangleRenderCommand command(spk::Rect2D(0, 0, 8, 8), spk::Color(1.0f, 0.0f, 0.0f));
	});
}

TEST(ColorRectangleRenderCommandTest, CanExecuteTwiceWithConstructedMesh)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::RenderContext& renderContext = context.renderContext();

	Viewport viewport(spk::Rect2D(0, 0, 16, 16));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, 16, 16), spk::Color(1.0f, 0.0f, 0.0f));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 16, 16),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/twice_actual"),
		sparkle_test::renderCommandExpectedPath("ColorRectangleRenderCommand/twice_expected"),
		sparkle_test::renderCommandResultPath("ColorRectangleRenderCommand/twice_diff"));
}

