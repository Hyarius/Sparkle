#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_clear_command.hpp"
#include "opengl/spk_opengl_viewport_command.hpp"
#include "rendering/render_command/spk_image_render_command.hpp"
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

	[[nodiscard]] std::shared_ptr<spk::OpenGL::Texture> makeSolidTexture(
		const spk::Vector2UInt& p_size,
		std::uint8_t p_red,
		std::uint8_t p_green,
		std::uint8_t p_blue,
		std::uint8_t p_alpha = 255)
	{
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(p_size.x) * static_cast<std::size_t>(p_size.y) * 4, 0);
		for (std::size_t i = 0; i < pixels.size(); i += 4)
		{
			pixels[i + 0] = p_red;
			pixels[i + 1] = p_green;
			pixels[i + 2] = p_blue;
			pixels[i + 3] = p_alpha;
		}

		auto texture = std::make_shared<spk::OpenGL::Texture>();
		texture->setPixels(
			pixels,
			p_size,
			spk::Texture::Format::RGBA,
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::ClampToEdge,
			spk::Texture::Mipmap::Disable);
		return texture;
	}
}

TEST(ImageRenderCommandTest, DrawsFullTextureWithWholeSection)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	auto blueTexture = makeSolidTexture({2, 2}, 0, 0, 255);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*blueTexture, spk::Texture::Section::whole, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	glFinish();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		resultPath("image_cmd_full_actual"),
		expectedPath("image_cmd_full_expected"),
		resultPath("image_cmd_full_diff"));
}

TEST(ImageRenderCommandTest, DrawsPartialTextureSection)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	// 2×1 texture: left pixel red, right pixel green
	std::vector<std::uint8_t> pixels = {255, 0, 0, 255, 0, 255, 0, 255};
	auto texture = std::make_shared<spk::OpenGL::Texture>();
	texture->setPixels(
		pixels,
		spk::Vector2UInt{2, 1},
		spk::Texture::Format::RGBA,
		spk::Texture::Filtering::Nearest,
		spk::Texture::Wrap::ClampToEdge,
		spk::Texture::Mipmap::Disable);

	// Section covering only the right (green) pixel
	const spk::Texture::Section greenSection({0.5f, 0.0f}, {0.5f, 1.0f});

	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, width, height));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*texture, greenSection, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	glFinish();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		resultPath("image_cmd_section_actual"),
		expectedPath("image_cmd_section_expected"),
		resultPath("image_cmd_section_diff"));
}

TEST(ImageRenderCommandTest, CanExecuteTwiceWithCachedMesh)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::IRenderContext& renderContext = context.renderContext();

	auto redTexture = makeSolidTexture({1, 1}, 255, 0, 0);
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::OpenGL::ViewportCommand>(spk::Rect2D(0, 0, 16, 16));
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*redTexture, spk::Texture::Section::whole, spk::Rect2D(0, 0, 16, 16));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
}

#endif
