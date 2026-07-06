#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <vector>

#include <GL/glew.h>

#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/render_command_test_utils.hpp"
#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"
#include "structures/graphics/rendering/command/spk_draw_texture_mesh_render_command.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "utils/image_comparison_test_utils.hpp"

using ClearCommand = spk::ClearCommand;
using Viewport = spk::Viewport;

namespace
{
	[[nodiscard]] spk::Vector3 toPosition(const spk::Vector2Int &p_pixel, float p_depth)
	{
		return {
			static_cast<float>(p_pixel.x),
			static_cast<float>(p_pixel.y),
			p_depth};
	}

	[[nodiscard]] spk::TextureMesh2D makeFullScreenMesh(const spk::Vector2UInt &p_size)
	{
		spk::TextureMesh2D::Builder builder;
		builder.addShape(
			spk::TextureVertex2D{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
			spk::TextureVertex2D{{0.0f, static_cast<float>(p_size.y), 0.0f}, {0.0f, 1.0f}},
			spk::TextureVertex2D{{static_cast<float>(p_size.x), static_cast<float>(p_size.y), 0.0f}, {1.0f, 1.0f}},
			spk::TextureVertex2D{{static_cast<float>(p_size.x), 0.0f, 0.0f}, {1.0f, 0.0f}});
		return builder.bake();
	}

	[[nodiscard]] spk::TextureMesh2D makeFontMesh(
		spk::Font::Atlas &p_atlas,
		const spk::Font::Text &p_text,
		const spk::Vector2Int &p_baselinePosition,
		float p_depth = 0.0f)
	{
		p_atlas.loadGlyphs(p_text);

		spk::TextureMesh2D::Builder builder;
		int cursorX = p_baselinePosition.x;

		for (spk::Font::Codepoint character : p_text)
		{
			const spk::Font::Glyph &glyph = p_atlas.glyph(character);

			if (glyph.size.x != 0 && glyph.size.y != 0)
			{
				std::array<spk::TextureVertex2D, 4> vertices;

				for (std::size_t i = 0; i < vertices.size(); ++i)
				{
					const spk::Vector2Int pixelPosition = {
						cursorX + glyph.positions[i].x,
						p_baselinePosition.y + glyph.positions[i].y};

					vertices[i] = {
						toPosition(pixelPosition, p_depth),
						glyph.uvs[i]};
				}

				builder.addShape(vertices[0], vertices[1], vertices[3], vertices[2]);
			}

			cursorX += glyph.step.x;
		}

		return builder.bake();
	}

	[[nodiscard]] spk::TextureMesh2D makeFontMesh(
		spk::Font::Atlas &p_atlas,
		std::string_view p_text,
		const spk::Vector2Int &p_baselinePosition,
		float p_depth = 0.0f)
	{
		return makeFontMesh(p_atlas, spk::Font::textFromUTF8(p_text), p_baselinePosition, p_depth);
	}
}

TEST(TextureMesh2DTest, StoresVector3PositionsAndUVs)
{
	spk::TextureMesh2D::Builder builder;
	builder.addShape(
		spk::TextureVertex2D{{0.0f, 0.0f, 0.25f}, {0.0f, 0.0f}},
		spk::TextureVertex2D{{1.0f, 0.0f, 0.25f}, {1.0f, 0.0f}},
		spk::TextureVertex2D{{1.0f, 1.0f, 0.25f}, {1.0f, 1.0f}});
	spk::TextureMesh2D mesh = builder.bake();

	ASSERT_EQ(mesh.vertices().size(), 3u);
	EXPECT_EQ(mesh.vertices()[0].position, (spk::Vector3{0.0f, 0.0f, 0.25f}));
	EXPECT_EQ(mesh.vertices()[2].uv, (spk::Vector2{1.0f, 1.0f}));
	EXPECT_EQ(mesh.indexes().size(), 3u);
}

TEST(TextureMesh2DTest, QuadShapeStoresFourVerticesAndSixIndexes)
{
	spk::TextureMesh2D mesh = makeFullScreenMesh({24, 24});

	EXPECT_EQ(mesh.nbShape(), 1u);
	EXPECT_EQ(mesh.vertices().size(), 4u);
	ASSERT_EQ(mesh.indexes().size(), 6u);
	EXPECT_EQ(mesh.indexes()[0], 0u);
	EXPECT_EQ(mesh.indexes()[1], 1u);
	EXPECT_EQ(mesh.indexes()[2], 2u);
	EXPECT_EQ(mesh.indexes()[3], 0u);
	EXPECT_EQ(mesh.indexes()[4], 2u);
	EXPECT_EQ(mesh.indexes()[5], 3u);
}

TEST(TextureMesh2DTest, ReserveVectorShapeAndClearUpdateStorage)
{
	spk::TextureMesh2D::Builder builder;
	builder.reserve(4, 6);

	const std::vector<spk::TextureVertex2D> vertices = {
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}};
	builder.addShape(vertices);
	spk::TextureMesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.nbShape(), 1u);
	EXPECT_EQ(mesh.vertices().size(), 4u);
	EXPECT_EQ(mesh.indexes().size(), 6u);

	spk::TextureMesh2D::Builder clearedBuilder;
	clearedBuilder.addShape(vertices);
	clearedBuilder.clear();
	spk::TextureMesh2D clearedMesh = clearedBuilder.bake();

	EXPECT_EQ(clearedMesh.nbShape(), 0u);
	EXPECT_TRUE(clearedMesh.vertices().empty());
	EXPECT_TRUE(clearedMesh.indexes().empty());
}

TEST(TextureMesh2DTest, DegenerateShapeIsIgnored)
{
	spk::TextureMesh2D::Builder builder;
	const std::vector<spk::TextureVertex2D> vertices = {
		{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}};

	EXPECT_THROW(builder.addShape(vertices), std::runtime_error);
	spk::TextureMesh2D mesh = builder.bake();

	EXPECT_EQ(mesh.nbShape(), 0u);
	EXPECT_TRUE(mesh.vertices().empty());
	EXPECT_TRUE(mesh.indexes().empty());
}

TEST(DrawTextureMeshRenderCommandTest, DrawsFullScreenTexture)
{
	constexpr int width = 24;
	constexpr int height = 24;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext &renderContext = context.renderContext();

	auto blueTexture = sparkle_test::makeSolidTexture({2, 2}, 0, 0, 255);
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(
		*blueTexture,
		makeFullScreenMesh({width, height}));

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
	spk::RenderContext &renderContext = context.renderContext();

	auto whiteTexture = sparkle_test::makeSolidTexture({1, 1}, 255, 255, 255);
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(*whiteTexture, spk::TextureMesh2D());

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
	spk::RenderContext &renderContext = context.renderContext();

	auto font = std::make_shared<spk::Font>(sparkle_test::testFont());

	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

	const spk::Font::Size smallSize(16, 0);
	std::shared_ptr<spk::Font::Atlas> smallAtlas = font->atlas(smallSize);
	builder.emplace<spk::DrawFontRenderCommand>(
		*smallAtlas,
		makeFontMesh(*smallAtlas, "A", spk::Vector2Int{4, 34}),
		smallSize,
		spk::Color(1.0f, 1.0f, 1.0f, 1.0f));

	const spk::Font::Size outlineSize(28, 3);
	std::shared_ptr<spk::Font::Atlas> outlineAtlas = font->atlas(outlineSize);
	builder.emplace<spk::DrawFontRenderCommand>(
		*outlineAtlas,
		makeFontMesh(*outlineAtlas, "A", spk::Vector2Int{40, 34}),
		outlineSize,
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
	spk::RenderContext &renderContext = context.renderContext();

	auto font = std::make_shared<spk::Font>(sparkle_test::testFont());
	Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

	const spk::Font::Size size(16, 0);
	std::shared_ptr<spk::Font::Atlas> atlas = font->atlas(size);
	builder.emplace<spk::DrawFontRenderCommand>(
		*atlas,
		makeFontMesh(*atlas, "", spk::Vector2Int{0, 16}),
		size);

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
