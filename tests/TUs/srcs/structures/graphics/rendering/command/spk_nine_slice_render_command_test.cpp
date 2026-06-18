#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

namespace
{
	[[nodiscard]] spk::SpriteSheet makeNineSpriteSheet()
	{
		const auto& bytes = SPARKLE_GET_RESOURCE("resources/textures/default_nine_slice.png");
		return spk::SpriteSheet::fromRawData(
			std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
			{3, 3},
			spk::Texture::Filtering::Nearest,
			spk::Texture::Wrap::ClampToEdge,
			spk::Texture::Mipmap::Disable);
	}
}

TEST(NineSliceRenderCommandTest, RejectsSpriteSheetsThatAreNotThreeByThree)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::SpriteSheet spriteSheet;
	spriteSheet.loadFromData(sparkle_test::makeTwoSpritePngBytes(), {2, 1});

	EXPECT_THROW(spk::NineSliceRenderCommand(spriteSheet, spk::Rect2D(0, 0, 16, 16), {2, 2}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, RejectsInvalidCornerSizes)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 16, 16));
	spk::SpriteSheet spriteSheet = makeNineSpriteSheet();

	EXPECT_THROW(spk::NineSliceRenderCommand(spriteSheet, spk::Rect2D(0, 0, 16, 16), {-1, 2}), std::invalid_argument);
	EXPECT_THROW(spk::NineSliceRenderCommand(spriteSheet, spk::Rect2D(0, 0, 16, 16), {9, 2}), std::invalid_argument);
}

TEST(NineSliceRenderCommandTest, BuildsAndDrawsTheNineSliceMesh)
{
	constexpr int width = 96;
	constexpr int height = 72;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();
	spk::SpriteSheet spriteSheet = makeNineSpriteSheet();

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::NineSliceRenderCommand>(spriteSheet, spk::Rect2D(0, 0, width, height), spk::Vector2Int{16, 16});

	EXPECT_NO_THROW(builder.build().execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("NineSliceRenderCommand/stretched_actual"),
		sparkle_test::renderCommandExpectedPath("NineSliceRenderCommand/stretched_expected"),
		sparkle_test::renderCommandResultPath("NineSliceRenderCommand/stretched_diff"));
}
