#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"
#include "render_command_test_utils.hpp"
#include "rendering/render_command/spk_draw_font_render_command.hpp"
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"
#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"

namespace
{
	[[nodiscard]] spk::TextureMesh2D makeFullScreenMesh(const spk::Vector2UInt& p_size)
	{
		spk::TextureMesh2D mesh;
		mesh.addShape(
			spk::TextureVertex2D{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			spk::TextureVertex2D{{0.0f, static_cast<float>(p_size.y), 0.0f}, {0.0f, 1.0f}},
			spk::TextureVertex2D{{static_cast<float>(p_size.x), static_cast<float>(p_size.y), 0.0f}, {1.0f, 1.0f}},
			spk::TextureVertex2D{{static_cast<float>(p_size.x), 0.0f, 0.0f}, {1.0f, 0.0f}});
		return mesh;
	}
}

TEST(TextureMesh2DTest, StoresVector3PositionsAndUVs)
{
	spk::TextureMesh2D mesh;
	mesh.addShape(
		spk::TextureVertex2D{{0.0f, 0.0f, 0.25f}, {0.0f, 0.0f}},
		spk::TextureVertex2D{{1.0f, 0.0f, 0.25f}, {1.0f, 0.0f}},
		spk::TextureVertex2D{{1.0f, 1.0f, 0.25f}, {1.0f, 1.0f}});

	ASSERT_EQ(mesh.buffer().vertices.size(), 3u);
	EXPECT_EQ(mesh.buffer().vertices[0].position, (spk::Vector3{0.0f, 0.0f, 0.25f}));
	EXPECT_EQ(mesh.buffer().vertices[2].uv, (spk::Vector2{1.0f, 1.0f}));
	EXPECT_EQ(mesh.buffer().indexes.size(), 3u);
}

TEST(TextureMesh2DTest, QuadShapeStoresFourVerticesAndSixIndexes)
{
	spk::TextureMesh2D mesh = makeFullScreenMesh({24, 24});

	EXPECT_EQ(mesh.nbShape(), 1u);
	ASSERT_EQ(mesh.shapes().size(), 1u);
	EXPECT_EQ(mesh.shapes()[0].size(), 4u);
	EXPECT_EQ(mesh.buffer().vertices.size(), 4u);
	ASSERT_EQ(mesh.buffer().indexes.size(), 6u);
	EXPECT_EQ(mesh.buffer().indexes[0], 0u);
	EXPECT_EQ(mesh.buffer().indexes[1], 1u);
	EXPECT_EQ(mesh.buffer().indexes[2], 2u);
	EXPECT_EQ(mesh.buffer().indexes[3], 0u);
	EXPECT_EQ(mesh.buffer().indexes[4], 2u);
	EXPECT_EQ(mesh.buffer().indexes[5], 3u);
}

TEST(TextureMesh2DTest, ReserveVectorShapeAndClearUpdateStorage)
{
	spk::TextureMesh2D mesh;
	mesh.reserve(4, 6);

	const std::vector<spk::TextureVertex2D> vertices = {
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
	};
	mesh.addShape(vertices);

	EXPECT_EQ(mesh.nbShape(), 1u);
	EXPECT_EQ(mesh.buffer().vertices.size(), 4u);
	EXPECT_EQ(mesh.buffer().indexes.size(), 6u);

	mesh.clear();

	EXPECT_EQ(mesh.nbShape(), 0u);
	EXPECT_TRUE(mesh.shapes().empty());
	EXPECT_TRUE(mesh.buffer().vertices.empty());
	EXPECT_TRUE(mesh.buffer().indexes.empty());
}

TEST(TextureMesh2DTest, DegenerateShapeIsIgnored)
{
	spk::TextureMesh2D mesh;
	const std::vector<spk::TextureVertex2D> vertices = {
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}
	};

	mesh.addShape(vertices);

	EXPECT_EQ(mesh.nbShape(), 0u);
	EXPECT_TRUE(mesh.buffer().vertices.empty());
	EXPECT_TRUE(mesh.buffer().indexes.empty());
}

TEST(DrawTextureMeshRenderCommandTest, DrawsFullScreenTexture)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	auto blueTexture = sparkle_test::makeSolidTexture({2, 2}, 0, 0, 255);
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(*blueTexture, makeFullScreenMesh({width, height}));

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("texture_mesh_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("texture_mesh_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("texture_mesh_diff");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	sparkle_test::writeSolidExpectedImage(expected, width, height, {0, 0, 255, 255});

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches)
		<< "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
}

TEST(DrawTextureMeshRenderCommandTest, EmptyMeshDoesNotDraw)
{
	sparkle_test::OpenGLTestContext context;
	spk::IRenderContext& renderContext = context.renderContext();

	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, 32, 32));
	spk::ViewportCommand viewportSetup(viewport);
	viewportSetup.execute(renderContext);

	auto whiteTexture = sparkle_test::makeSolidTexture({1, 1}, 255, 255, 255);
	spk::DrawTextureMeshRenderCommand command(*whiteTexture, spk::TextureMesh2D{});

	EXPECT_NO_THROW(command.execute(renderContext));
}

TEST(DrawFontRenderCommandTest, DrawsGlyphsWithSizeAndOutline)
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
	builder.emplace<spk::DrawFontRenderCommand>(
		font,
		L"A",
		spk::Vector2Int{4, 34},
		spk::Font::Size(16, 0),
		spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	builder.emplace<spk::DrawFontRenderCommand>(
		font,
		L"A",
		spk::Vector2Int{40, 34},
		spk::Font::Size(28, 3),
		spk::Color(1.0f, 1.0f, 1.0f, 1.0f));

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("font_actual");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));

	EXPECT_GT(sparkle_test::countLitPixels(actual), 100u);
}

TEST(DrawFontRenderCommandTest, EmptyTextDoesNotDraw)
{
	sparkle_test::OpenGLTestContext context;
	spk::Font font = sparkle_test::testFont();
	spk::DrawFontRenderCommand command(font, L"", {0, 16}, spk::Font::Size(16, 0));

	EXPECT_NO_THROW(command.execute(context.renderContext()));
}

#endif
