#include <gtest/gtest.h>


#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>

#include <GL/glew.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/command/spk_image_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

using ClearCommand = spk::ClearCommand;
using Texture = spk::Texture;
using Viewport = spk::Viewport;

TEST(ImageRenderCommandTest, DrawsFullTextureWithWholeSection)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	auto blueTexture = sparkle_test::makeSolidTexture({2, 2}, 0, 0, 255);
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*blueTexture, spk::Texture::Section::whole, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/full_actual"),
		sparkle_test::renderCommandExpectedPath("ImageRenderCommand/full_expected"),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/full_diff"));
}

TEST(ImageRenderCommandTest, DrawsPartialTextureSection)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	// 2x1 texture: left pixel red, right pixel green
	std::vector<std::uint8_t> pixels = {255, 0, 0, 255, 0, 255, 0, 255};
	auto texture = std::make_shared<Texture>();
	texture->setPixels(
		pixels,
		spk::Vector2UInt{2, 1},
		spk::Texture::Format::RGBA,
		spk::Texture::Filtering::Nearest,
		spk::Texture::Wrap::ClampToEdge,
		spk::Texture::Mipmap::Disable);

	// Section covering only the right (green) pixel
	const spk::Texture::Section greenSection({0.5f, 0.0f}, {0.5f, 1.0f});

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*texture, greenSection, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/section_actual"),
		sparkle_test::renderCommandExpectedPath("ImageRenderCommand/section_expected"),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/section_diff"));
}

TEST(ImageRenderCommandTest, CanExecuteTwiceWithConstructedMesh)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::RenderContext& renderContext = context.renderContext();

	auto redTexture = sparkle_test::makeSolidTexture({1, 1}, 255, 0, 0);
	Viewport viewport(spk::Rect2D(0, 0, 16, 16));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ImageRenderCommand>(*redTexture, spk::Texture::Section::whole, spk::Rect2D(0, 0, 16, 16));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 16, 16),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/twice_actual"),
		sparkle_test::renderCommandExpectedPath("ImageRenderCommand/twice_expected"),
		sparkle_test::renderCommandResultPath("ImageRenderCommand/twice_diff"));
}

