#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include <GL/glew.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/command/spk_color_rectangle_render_command.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"

// Depth contract under test: vertex z is the layer, valid in [0, maxLayer], and a
// HIGHER layer always covers a lower one regardless of draw order. The hidden test
// window's back buffer has undefined pixel ownership, so every capture goes through
// a dedicated offscreen framebuffer and pixels are asserted directly.

namespace
{
	using sparkle_test::OffscreenRenderTarget;

	void expectPixelRGB(int p_x, int p_y, std::uint8_t p_red, std::uint8_t p_green, std::uint8_t p_blue)
	{
		const std::array<std::uint8_t, 4> pixel = sparkle_test::readPixel(p_x, p_y);
		EXPECT_EQ(static_cast<int>(pixel[0]), static_cast<int>(p_red)) << "red at (" << p_x << ", " << p_y << ")";
		EXPECT_EQ(static_cast<int>(pixel[1]), static_cast<int>(p_green)) << "green at (" << p_x << ", " << p_y << ")";
		EXPECT_EQ(static_cast<int>(pixel[2]), static_cast<int>(p_blue)) << "blue at (" << p_x << ", " << p_y << ")";
	}

	const spk::Color red(1.0f, 0.0f, 0.0f);
	const spk::Color green(0.0f, 1.0f, 0.0f);
	const spk::Color blue(0.0f, 0.0f, 1.0f);
}

TEST(DepthOrderingRenderTest, DepthZeroIsVisibleOnClearedTarget)
{
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 0.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 255, 0, 0);
}

TEST(DepthOrderingRenderTest, PositiveDepthIsVisibleOnClearedTarget)
{
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), green, 1.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 0, 255, 0);
}

TEST(DepthOrderingRenderTest, DepthOneCoversDepthZeroWhenDrawnFirst)
{
	// The depth-1 rectangle is drawn BEFORE the depth-0 one: only the depth buffer
	// can keep it on top, draw order would yield the opposite result.
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 1.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), green, 0.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 255, 0, 0);
}

TEST(DepthOrderingRenderTest, DepthOneCoversDepthZeroWhenDrawnLast)
{
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), green, 0.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 1.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 255, 0, 0);
}

TEST(DepthOrderingRenderTest, IntermediateDepthsStackByLayerNotByDrawOrder)
{
	// Staircase overlap, drawn in scrambled order (5, 10, 0):
	//   left third  -> only the depth-10 rect          -> green
	//   middle      -> depth-10 over depth-5 (and 0)   -> green
	//   right third -> depth-5 over depth-0            -> blue
	constexpr int width = 18;
	constexpr int height = 12;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(6, 0, 12, height), blue, 5.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, 12, height), green, 10.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 0.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(3, height / 2, 0, 255, 0);
	expectPixelRGB(9, height / 2, 0, 255, 0);
	expectPixelRGB(15, height / 2, 0, 0, 255);
}

TEST(DepthOrderingRenderTest, EqualDepthsResolveByDrawOrder)
{
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), green, 2.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 2.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 255, 0, 0);
}

TEST(DepthOrderingRenderTest, TopLayerIsVisibleAndCoversLowerLayers)
{
	// maxLayer itself sits exactly on the near plane, where float rounding of the
	// projection can clip it: the highest reliably usable layer is just below it.
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), blue, spk::Viewport::maxLayer() - 1.0f);
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, 500.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 0, 0, 255);
}

TEST(DepthOrderingRenderTest, DepthBeyondMaxLayerIsClipped)
{
	// Documents the valid layer range [0, maxLayer]: anything farther than the
	// near plane is clipped away instead of wrapping back into the scene.
	constexpr int width = 16;
	constexpr int height = 16;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	OffscreenRenderTarget target(width, height);
	ASSERT_TRUE(target.isComplete());

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::ColorRectangleRenderCommand>(spk::Rect2D(0, 0, width, height), red, spk::Viewport::maxLayer() * 2.0f);

	builder.build().execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	expectPixelRGB(width / 2, height / 2, 0, 0, 0);
}
