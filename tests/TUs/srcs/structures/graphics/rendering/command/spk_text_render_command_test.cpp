#include <gtest/gtest.h>


#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include <GL/glew.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/command/spk_text_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

TEST(TextRenderCommandTest, DrawsGlyphsWithLeftTopAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16, 3), spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
		spk::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.0f,
		spk::Vector2Int{2, 2},
		spk::HorizontalAlignment::Left,
		spk::VerticalAlignment::Top);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("TextRenderCommand/left_top_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("TextRenderCommand/left_top_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("TextRenderCommand/left_top_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, DrawsGlyphsWithCenteredAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16, 3), spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
		spk::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.0f,
		spk::Vector2Int{width / 2, height / 2},
		spk::HorizontalAlignment::Centered,
		spk::VerticalAlignment::Centered);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("TextRenderCommand/centered_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("TextRenderCommand/centered_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("TextRenderCommand/centered_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, DrawsGlyphsWithRightDownAlignment)
{
	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16, 3), spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
		spk::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.0f,
		spk::Vector2Int{width - 2, height - 2},
		spk::HorizontalAlignment::Right,
		spk::VerticalAlignment::Down);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("TextRenderCommand/right_down_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("TextRenderCommand/right_down_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("TextRenderCommand/right_down_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(TextRenderCommandTest, EmptyTextDoesNotDraw)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 32, 32));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, 32, 32));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "", spk::Font::Size(16, 3), spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
		spk::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.0f,
		spk::Vector2Int{0, 0});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 32, 32),
		sparkle_test::renderCommandResultPath("TextRenderCommand/empty_actual"),
		sparkle_test::renderCommandExpectedPath("TextRenderCommand/empty_expected"),
		sparkle_test::renderCommandResultPath("TextRenderCommand/empty_diff"));
}

TEST(TextRenderCommandTest, CanExecuteTwiceWithConstructedCommand)
{
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, 80, 48));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, 80, 48));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::TextRenderCommand>(
		font, "Hi", spk::Font::Size(16, 3), spk::Color(0.0f, 1.0f, 0.0f, 1.0f),
		spk::Color(1.0f, 0.0f, 0.0f, 1.0f), 0.0f,
		spk::Vector2Int{2, 2});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, 80, 48),
		sparkle_test::renderCommandResultPath("TextRenderCommand/twice_actual"),
		sparkle_test::renderCommandExpectedPath("TextRenderCommand/twice_expected"),
		sparkle_test::renderCommandResultPath("TextRenderCommand/twice_diff"));
}

TEST(TextRenderCommandTest, RebuildsDrawCommandAfterAtlasGrowth)
{
	// Regression test for stale glyph UVs: TextRenderCommand owns the text to
	// mesh conversion and must recreate its draw command after the atlas grows.
	constexpr int width = 64;
	constexpr int height = 40;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	sparkle_test::OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Font font = sparkle_test::testFont();
	const spk::Font::Size size(16, 0);
	const spk::Color glyphColor(1.0f, 1.0f, 1.0f, 1.0f);
	const spk::Color outlineColor(0.0f, 0.0f, 0.0f, 0.0f);

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder cachedBuilder;
	cachedBuilder.emplace<spk::ViewportCommand>(viewport);
	cachedBuilder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	cachedBuilder.emplace<spk::TextRenderCommand>(
		font,
		"AB",
		size,
		glyphColor,
		outlineColor,
		0.0f,
		spk::Vector2Int{4, 28});
	spk::RenderUnit cachedUnit = cachedBuilder.build();

	cachedUnit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	spk::Font::Atlas& atlas = font.atlas(size);
	const spk::Vector2UInt atlasSizeBefore = atlas.size();
	atlas.loadGlyphs(sparkle_test::manyGlyphsText());
	ASSERT_TRUE(atlas.size() != atlasSizeBefore) << "the glyph load did not grow the atlas; the test premise is broken";

	cachedUnit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();
	const std::vector<std::uint8_t> cachedCommandPixels = sparkle_test::readPixels(width, height);

	spk::RenderUnitBuilder freshBuilder;
	freshBuilder.emplace<spk::ViewportCommand>(viewport);
	freshBuilder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	freshBuilder.emplace<spk::TextRenderCommand>(
		font,
		"AB",
		size,
		glyphColor,
		outlineColor,
		0.0f,
		spk::Vector2Int{4, 28});
	freshBuilder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();
	const std::vector<std::uint8_t> freshCommandPixels = sparkle_test::readPixels(width, height);

	std::size_t differingBytes = 0;
	for (std::size_t i = 0; i < cachedCommandPixels.size(); ++i)
	{
		if (cachedCommandPixels[i] != freshCommandPixels[i])
		{
			++differingBytes;
		}
	}
	EXPECT_EQ(differingBytes, 0u) << "cached text command drew with stale atlas UVs";

	bool anyGlyphPixel = false;
	for (std::size_t i = 0; i < cachedCommandPixels.size(); i += 4)
	{
		if (cachedCommandPixels[i] != 0)
		{
			anyGlyphPixel = true;
			break;
		}
	}
	EXPECT_TRUE(anyGlyphPixel) << "no text was rendered at all; the comparison is vacuous";
}
