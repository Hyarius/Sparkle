#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstddef>
#include <filesystem>

#include <GL/glew.h>

#include "opengl_wrapper_test_utils.hpp"
#include "render_command_test_utils.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"
#include "rendering/render_command/spk_text_render_command.hpp"
#include "rendering/spk_render_unit_builder.hpp"

TEST(TextRenderCommandTest, DrawsGlyphsWithLeftTopAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
		spk::Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f,
		spk::Vector2Int{2, 2},
		spk::HorizontalAlignment::Left,
		spk::VerticalAlignment::Top);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("text_cmd_left_top_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("text_cmd_left_top_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("text_cmd_left_top_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, DrawsGlyphsWithCenteredAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
		spk::Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f,
		spk::Vector2Int{width / 2, height / 2},
		spk::HorizontalAlignment::Centered,
		spk::VerticalAlignment::Centered);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("text_cmd_centered_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("text_cmd_centered_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("text_cmd_centered_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, DrawsGlyphsWithRightDownAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
		spk::Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f,
		spk::Vector2Int{width - 2, height - 2},
		spk::HorizontalAlignment::Right,
		spk::VerticalAlignment::Down);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("text_cmd_right_down_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("text_cmd_right_down_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("text_cmd_right_down_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, EmptyTextDoesNotDraw)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 32, 32));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 32, 32));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
		spk::Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f,
		spk::Vector2Int{0, 0});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 32, 32),
		sparkle_test::renderCommandResultPath("text_cmd_empty_actual"),
		sparkle_test::renderCommandExpectedPath("text_cmd_empty_expected"),
		sparkle_test::renderCommandResultPath("text_cmd_empty_diff"));
}

TEST(TextRenderCommandTest, CanExecuteTwiceWithConstructedCommand)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 80, 48));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 80, 48));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f),
		spk::Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f,
		spk::Vector2Int{2, 2});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 80, 48),
		sparkle_test::renderCommandResultPath("text_cmd_twice_actual"),
		sparkle_test::renderCommandExpectedPath("text_cmd_twice_expected"),
		sparkle_test::renderCommandResultPath("text_cmd_twice_diff"));
}

#endif
