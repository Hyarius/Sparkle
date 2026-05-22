#include <gtest/gtest.h>

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include <GL/glew.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "image_comparison_test_utils.hpp"
#include "opengl_wrapper_test_utils.hpp"
#include "rendering/render_command/spk_draw_font_render_command.hpp"
#include "rendering/render_command/spk_draw_texture_mesh_render_command.hpp"
#include "rendering/spk_texture_mesh_2d.hpp"
#include "opengl/spk_opengl_viewport.hpp"
#include "rendering/render_command/spk_viewport_render_command.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path renderCommandExpectedImagePath(const std::string& p_name)
	{
		std::filesystem::path directory = spk::test::expectedDirectory() / "RenderCommands";
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

	[[nodiscard]] std::filesystem::path renderCommandResultImagePath(const std::string& p_name)
	{
		std::filesystem::path directory = spk::test::resultDirectory() / "RenderCommands";
		std::filesystem::create_directories(directory);
		return directory / (p_name + ".png");
	}

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

	[[nodiscard]] std::shared_ptr<spk::OpenGL::Texture> makeSolidTexture(
		const spk::Vector2UInt& p_size,
		std::uint8_t p_red,
		std::uint8_t p_green,
		std::uint8_t p_blue,
		std::uint8_t p_alpha = 255)
	{
		std::vector<std::uint8_t> pixels(
			static_cast<std::size_t>(p_size.x) * static_cast<std::size_t>(p_size.y) * 4,
			0);
		for (std::size_t index = 0; index < pixels.size(); index += 4)
		{
			pixels[index + 0] = p_red;
			pixels[index + 1] = p_green;
			pixels[index + 2] = p_blue;
			pixels[index + 3] = p_alpha;
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

	[[nodiscard]] std::vector<std::uint8_t> makeTwoSpritePngBytes()
	{
		constexpr int width = 32;
		constexpr int height = 16;
		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const std::size_t index = (static_cast<std::size_t>(y) * width + static_cast<std::size_t>(x)) * 4;
				const bool rightSprite = x >= width / 2;
				pixels[index + 0] = rightSprite ? 0 : 255;
				pixels[index + 1] = rightSprite ? 255 : 0;
				pixels[index + 2] = 0;
				pixels[index + 3] = 255;
			}
		}

		std::vector<std::uint8_t> result;
		stbi_write_png_to_func(
			[](void* p_context, void* p_data, int p_size)
			{
				auto* output = reinterpret_cast<std::vector<std::uint8_t>*>(p_context);
				const auto* bytes = reinterpret_cast<const std::uint8_t*>(p_data);
				output->insert(output->end(), bytes, bytes + p_size);
			},
			&result,
			width,
			height,
			4,
			pixels.data(),
			width * 4);
		return result;
	}

	[[nodiscard]] std::filesystem::path systemFontPath()
	{
		const std::filesystem::path arial = "C:/Windows/Fonts/arial.ttf";
		if (std::filesystem::exists(arial))
		{
			return arial;
		}

		const std::filesystem::path segoe = "C:/Windows/Fonts/segoeui.ttf";
		if (std::filesystem::exists(segoe))
		{
			return segoe;
		}

		return {};
	}

	[[nodiscard]] std::size_t countLitPixels(const std::filesystem::path& p_path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char* rawPixels = stbi_load(p_path.string().c_str(), &width, &height, &channels, 4);
		if (rawPixels == nullptr)
		{
			throw std::runtime_error("Failed to load rendered image");
		}

		std::size_t result = 0;
		const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
		for (std::size_t i = 0; i < pixelCount; ++i)
		{
			const std::size_t index = i * 4;
			if (rawPixels[index + 0] > 10 || rawPixels[index + 1] > 10 || rawPixels[index + 2] > 10)
			{
				++result;
			}
		}
		stbi_image_free(rawPixels);
		return result;
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

	auto blueTexture = makeSolidTexture({2, 2}, 0, 0, 255);
	spk::OpenGL::Viewport viewport(spk::Rect2D(0, 0, width, height));
	spk::RenderUnitBuilder builder;
	builder.emplace<spk::ViewportCommand>(viewport);
	builder.emplace<spk::OpenGL::ClearCommand>(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
	builder.emplace<spk::DrawTextureMeshRenderCommand>(*blueTexture, makeFullScreenMesh({width, height}));

	spk::RenderUnit unit = builder.build();
	unit.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = renderCommandResultImagePath("texture_mesh_actual");
	const std::filesystem::path expected = renderCommandExpectedImagePath("texture_mesh_expected");
	const std::filesystem::path diff = renderCommandResultImagePath("texture_mesh_diff");
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

	auto whiteTexture = makeSolidTexture({1, 1}, 255, 255, 255);
	spk::DrawTextureMeshRenderCommand command(*whiteTexture, spk::TextureMesh2D{});

	EXPECT_NO_THROW(command.execute(renderContext));
}

TEST(DrawFontRenderCommandTest, DrawsGlyphsWithSizeAndOutline)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	constexpr int width = 80;
	constexpr int height = 48;
	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::IRenderContext& renderContext = context.renderContext();

	spk::Font font(fontPath);

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

	const std::filesystem::path actual = renderCommandResultImagePath("font_actual");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));

	EXPECT_GT(countLitPixels(actual), 100u);
}

TEST(DrawFontRenderCommandTest, EmptyTextDoesNotDraw)
{
	const std::filesystem::path fontPath = systemFontPath();
	if (fontPath.empty())
	{
		GTEST_SKIP() << "No known Windows system font found";
	}

	sparkle_test::OpenGLTestContext context;
	spk::Font font(fontPath);
	spk::DrawFontRenderCommand command(font, L"", {0, 16}, spk::Font::Size(16, 0));

	EXPECT_NO_THROW(command.execute(context.renderContext()));
}

#endif
