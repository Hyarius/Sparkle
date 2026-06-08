#include <gtest/gtest.h>


#include <array>
#include <cstddef>
#include <filesystem>

#include <GL/glew.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/command/spk_sprite_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

TEST(SpriteRenderCommandTest, DrawsSelectedSpriteByCoordinates)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{1, 0}, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/selected_actual"),
		sparkle_test::renderCommandExpectedPath("SpriteRenderCommand/selected_expected"),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/selected_diff"));
}

TEST(SpriteRenderCommandTest, DrawsFirstSpriteByCoordinates)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{0, 0}, spk::Rect2D(0, 0, width, height));

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/first_actual"),
		sparkle_test::renderCommandExpectedPath("SpriteRenderCommand/first_expected"),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/first_diff"));
}

TEST(SpriteRenderCommandTest, RejectsOutOfBoundsSpriteCoordinates)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 8, 8));
	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});

	EXPECT_THROW(
		spk::SpriteRenderCommand(spriteSheet, spk::Vector2UInt{5, 5}, spk::Rect2D(0, 0, 8, 8)),
		std::out_of_range);
}

TEST(SpriteRenderCommandTest, CanExecuteTwiceWithConstructedMesh)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::RenderContext& renderContext = context.renderContext();

	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});

	Viewport viewport(spk::Rect2D(0, 0, 16, 16));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::SpriteRenderCommand>(spriteSheet, spk::Vector2UInt{0, 0}, spk::Rect2D(0, 0, 16, 16));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 16, 16),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/twice_actual"),
		sparkle_test::renderCommandExpectedPath("SpriteRenderCommand/twice_expected"),
		sparkle_test::renderCommandResultPath("SpriteRenderCommand/twice_diff"));
}

