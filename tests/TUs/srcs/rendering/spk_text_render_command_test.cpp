#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstddef>
#include <filesystem>

#include <GL/glew.h>
#include <stb_image.h>

#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_viewport_command.hpp"
#include "rendering/render_command/spk_text_render_command.hpp"
#include "rendering/spk_render_unit_builder.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path resultPath(const std::string& p_name)
	{
		std::filesystem::path directory = spk::test::resultDirectory() / "RenderCommands";
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

	[[nodiscard]] std::filesystem::path systemFontPath()
	{
		const std::filesystem::path arial = "C:/Windows/Fonts/arial.ttf";
		if (std::filesystem::exists(arial))
		{
			return arial;
		}

		const std::filesystem::path segoe = "C:/Windows/Fonts/segoeui.ttf";
		if (std::filesystem::exists(segoe))
		{
			return segoe;
		}

		return {};
	}

	[[nodiscard]] std::size_t countLitPixels(const std::filesystem::path& p_path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* rawPixels = stbi_load(p_path.string().c_str(), &width, &height, &channels, 4);
		if (rawPixels == nullptr)
		{
			throw std::runtime_error("Failed to load rendered image: " + p_path.string());
		}

		std::size_t result = 0;
		const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
		for (std::size_t i = 0; i < pixelCount; ++i)
		{
			const std::size_t index = i * 4;
			if (rawPixels[index + 0] > 10 || rawPixels[index + 1] > 10 || rawPixels[index + 2] > 10)
			{
				++result;
			}
		}
		stbi_image_free(rawPixels);
		return result;
	}
}

TEST(TextRenderCommandTest, DrawsGlyphsWithLeftTopAlignment)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, L"Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
		spk::Vector2Int{2, 2},
		spk::HorizontalAlignment::Left,
		spk::VerticalAlignment::Top);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = resultPath("text_cmd_left_top_actual");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	EXPECT_GT(countLitPixels(actual), 0u) << "Expected at least some lit pixels for rendered text";
}

TEST(TextRenderCommandTest, DrawsGlyphsWithCenteredAlignment)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, L"Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
		spk::Vector2Int{width / 2, height / 2},
		spk::HorizontalAlignment::Centered,
		spk::VerticalAlignment::Centered);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = resultPath("text_cmd_centered_actual");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	EXPECT_GT(countLitPixels(actual), 0u) << "Expected at least some lit pixels for rendered text";
}

TEST(TextRenderCommandTest, DrawsGlyphsWithRightDownAlignment)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, L"Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
		spk::Vector2Int{width - 2, height - 2},
		spk::HorizontalAlignment::Right,
		spk::VerticalAlignment::Down);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	glFinish();

	const std::filesystem::path actual = resultPath("text_cmd_right_down_actual");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	EXPECT_GT(countLitPixels(actual), 0u) << "Expected at least some lit pixels for rendered text";
}

TEST(TextRenderCommandTest, EmptyTextDoesNotDraw)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 32, 32));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, 32, 32));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, L"", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
		spk::Vector2Int{0, 0});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
}

TEST(TextRenderCommandTest, CanExecuteTwiceWithCachedCommand)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 80, 48));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, 80, 48));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, L"Hi", spk::Font::Size(16), spk::Color(1.0f, 1.0f, 1.0f, 1.0f), 0.0f,
		spk::Vector2Int{2, 2});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
}

#endif
