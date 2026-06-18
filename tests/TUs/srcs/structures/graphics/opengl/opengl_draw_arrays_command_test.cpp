#include <gtest/gtest.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

#include "structures/graphics/rendering/state/spk_viewport.hpp"

#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

TEST(OpenGLDrawArraysCommandTest, DrawsConfiguredVertexRange)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	const auto program = sparkle_test::makeColorProgram();
	const auto vertexArray = sparkle_test::makeTriangleVAO(sparkle_test::fullScreenTriangle({0.0f, 1.0f, 0.0f}));

	spk::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawArraysCommand>(spk::Primitive::Triangles, program, vertexArray, 0, 3);

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("draw_arrays_command_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("draw_arrays_command_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("draw_arrays_command_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {0, 255, 0, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches);
}

TEST(OpenGLDrawArraysCommandTest, ExecutesWithoutOptionalProgramOrVertexArray)
{
	sparkle_test::OpenGLTestContext context;
	spk::DrawArraysCommand command(spk::Primitive::Points, 0, 0);

	EXPECT_NO_THROW(command.execute(context.renderContext()));
}
