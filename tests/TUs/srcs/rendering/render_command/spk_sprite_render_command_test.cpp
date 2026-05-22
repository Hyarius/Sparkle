#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <GL/glew.h>
#include <stb_image_write.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"
#include "rendering/render_command/spk_sprite_render_command.hpp"
#include "rendering/spk_render_unit_builder.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path expectedPath(const std::string& p_name)
	{
		std::filesystem::path directory = spk::test::expectedDirectory() / "RenderCommands";
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

	[[nodiscard]] std::filesystem::path resultPath(const std::string& p_name)
	{
		std::filesystem::path directory = spk::test::resultDirectory() / "RenderCommands";
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

	[[nodiscard]] std::vector<std::uint8_t> makeTwoSpritePngBytes()
	{
		constexpr int width = 32;
		constexpr int height = 16;
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const std::size_t index = (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4;
				const bool rightSprite = x >= width / 2;
				pixels[index + 0] = rightSprite ? 0u : 255u;
				pixels[index + 1] = rightSprite ? 255u : 0u;
				pixels[index + 2] = 0;
				pixels[index + 3] = 255;
			}
		}

		std::vector<std::uint8_t> result;
		stbi_write_png_to_func(
			[](void* p_context, void* p_data, int p_size)
			{
				auto* output = reinterpret_cast<std::vector<std::uint8_t>*>(p_context);
				const auto* bytes = reinterpret_cast<const std::uint8_t*>(p_data);
				output->insert(output->end(), bytes, bytes + p_size);
			},
			&result, width, height, 4, pixels.data(), width * 4);
		return result;
	}
}

TEST(SpriteRenderCommandTest, DrawsSelectedSpriteByCoordinates)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(makeTwoSpritePngBytes(), {2, 1});

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{1, 0}, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		resultPath("sprite_cmd_actual"),
		expectedPath("sprite_cmd_expected"),
		resultPath("sprite_cmd_diff"));
}

TEST(SpriteRenderCommandTest, DrawsFirstSpriteByCoordinates)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(makeTwoSpritePngBytes(), {2, 1});

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{0, 0}, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		resultPath("sprite_cmd_first_actual"),
		expectedPath("sprite_cmd_first_expected"),
		resultPath("sprite_cmd_first_diff"));
}

TEST(SpriteRenderCommandTest, RejectsOutOfBoundsSpriteCoordinates)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 8, 8));
	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(makeTwoSpritePngBytes(), {2, 1});

	EXPECT_THROW(
		spk::SpriteRenderCommand(spriteSheet, spk::Vector2UInt{5, 5}, spk::Rect2D(0, 0, 8, 8)),
		std::out_of_range);
}

TEST(SpriteRenderCommandTest, CanExecuteTwiceWithConstructedMesh)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(makeTwoSpritePngBytes(), {2, 1});

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 16, 16));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{0, 0}, spk::Rect2D(0, 0, 16, 16));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
}

#endif
