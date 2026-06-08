#include <gtest/gtest.h>


#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

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
	spk::RenderContext& renderContext = context.renderContext();

	auto blueTexture = sparkle_test::makeSolidTexture({2, 2}, 0, 0, 255);
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(*blueTexture, makeFullScreenMesh({width, height}));

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("DrawTextureMeshRenderCommand/full_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("DrawTextureMeshRenderCommand/full_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("DrawTextureMeshRenderCommand/full_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(DrawTextureMeshRenderCommandTest, EmptyMeshDoesNotDraw)
{
	constexpr int width = 32;
	constexpr int height = 32;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	auto whiteTexture = sparkle_test::makeSolidTexture({1, 1}, 255, 255, 255);
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(*whiteTexture, spk::TextureMesh2D{});

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("DrawTextureMeshRenderCommand/empty_actual"),
		sparkle_test::renderCommandExpectedPath("DrawTextureMeshRenderCommand/empty_expected"),
		sparkle_test::renderCommandResultPath("DrawTextureMeshRenderCommand/empty_diff"));
}

TEST(DrawFontRenderCommandTest, DrawsGlyphsWithSizeAndOutline)
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
	builder.emplace<spk::DrawFontRenderCommand>(
		font,
		"A",
		spk::Vector2Int{4, 34},
		spk::Font::Size(16, 0),
		spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	builder.emplace<spk::DrawFontRenderCommand>(
		font,
		"A",
		spk::Vector2Int{40, 34},
		spk::Font::Size(28, 3),
		spk::Color(1.0f, 1.0f, 1.0f, 1.0f));

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::renderCommandResultPath("DrawFontRenderCommand/size_outline_actual");
	const std::filesystem::path expected = sparkle_test::renderCommandExpectedPath("DrawFontRenderCommand/size_outline_expected");
	const std::filesystem::path diff = sparkle_test::renderCommandResultPath("DrawFontRenderCommand/size_outline_diff");
	sparkle_test::validateScreenshot(context, spk::Rect2D(0, 0, width, height), actual, expected, diff);
}

TEST(DrawFontRenderCommandTest, EmptyTextDoesNotDraw)
{
	constexpr int width = 32;
	constexpr int height = 32;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	spk::Font font = sparkle_test::testFont();
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawFontRenderCommand>(font, "", spk::Vector2Int{0, 16}, spk::Font::Size(16, 0));

	spk::RenderUnit unit = builder.build();
	EXPECT_NO_THROW(unit.execute(renderContext));
	context.gpuRuntime().waitUntilWorkDone();

	sparkle_test::validateScreenshot(
		context,
		spk::Rect2D(0, 0, width, height),
		sparkle_test::renderCommandResultPath("DrawFontRenderCommand/empty_actual"),
		sparkle_test::renderCommandExpectedPath("DrawFontRenderCommand/empty_expected"),
		sparkle_test::renderCommandResultPath("DrawFontRenderCommand/empty_diff"));
}

