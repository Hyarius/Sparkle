#include <array>
#include <filesystem>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image_write.h>

#include "utils/image_comparison_test_utils.hpp"
#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"

using BufferObject = spk::BufferObject;
using DrawArraysCommand = spk::DrawArraysCommand;
using Primitive = spk::Primitive;
using VertexArrayObject = spk::VertexArrayObject;
using VertexBufferObject = spk::VertexBufferObject;

namespace
{
	class SolidSquareWidget : public spk::Widget
	{
	private:
		std::shared_ptr<spk::Program> _program;
		std::shared_ptr<VertexArrayObject> _vertexArray;

		static std::shared_ptr<VertexArrayObject> makeSquareVAO(const std::array<float, 3>& p_color)
		{
			const std::array<sparkle_test::TestVertex, 6> vertices = {
				sparkle_test::TestVertex{{-1.0f, -1.0f}, p_color},
				sparkle_test::TestVertex{{1.0f, -1.0f}, p_color},
				sparkle_test::TestVertex{{1.0f, 1.0f}, p_color},
				sparkle_test::TestVertex{{-1.0f, -1.0f}, p_color},
				sparkle_test::TestVertex{{1.0f, 1.0f}, p_color},
				sparkle_test::TestVertex{{-1.0f, 1.0f}, p_color}
			};

			auto vertexBuffer = std::make_shared<VertexBufferObject>(
				BufferObject::Usage::StaticDraw,
				sizeof(vertices));
			vertexBuffer->edit(vertices.data(), sizeof(vertices));

			auto vertexArray = std::make_shared<VertexArrayObject>();
			vertexArray->addVertexBuffer(
				vertexBuffer,
				VertexArrayObject::Attribute{
					.index = 0,
					.componentCount = 2,
					.componentType = GL_FLOAT,
					.normalized = false,
					.stride = sizeof(sparkle_test::TestVertex),
					.offset = offsetof(sparkle_test::TestVertex, position)
				});
			vertexArray->addVertexBuffer(
				vertexBuffer,
				VertexArrayObject::Attribute{
					.index = 1,
					.componentCount = 3,
					.componentType = GL_FLOAT,
					.normalized = false,
					.stride = sizeof(sparkle_test::TestVertex),
					.offset = offsetof(sparkle_test::TestVertex, color)
				});
			return vertexArray;
		}

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			spk::RenderUnitBuilder builder;
			builder.emplace<DrawArraysCommand>(
				Primitive::Triangles,
				_program,
				_vertexArray,
				0,
				6);
			return builder.build();
		}

	public:
		SolidSquareWidget(const std::string& p_name, const std::array<float, 3>& p_color, spk::Widget* p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_program(sparkle_test::makeColorProgram()),
			_vertexArray(makeSquareVAO(p_color))
		{
			activate();
		}

		const spk::Rect2D& absoluteGeometryForTest() const
		{
			return absoluteGeometry();
		}

		const spk::Rect2D& scissorForTest() const
		{
			return scissor();
		}
	};

	struct ViewportScissorHierarchy
	{
		SolidSquareWidget parent{"Parent", {1.0f, 0.0f, 0.0f}};
		SolidSquareWidget edgeChild{"EdgeChild", {0.0f, 1.0f, 1.0f}, &parent};
		SolidSquareWidget clippedChild{"ClippedChild", {0.0f, 1.0f, 0.0f}, &parent};
		SolidSquareWidget grandChild{"GrandChild", {1.0f, 1.0f, 0.0f}, &clippedChild};
		SolidSquareWidget outsideChild{"OutsideChild", {1.0f, 0.0f, 1.0f}, &parent};

		ViewportScissorHierarchy()
		{
			parent.setGeometry(spk::Rect2D(2, 2, 12, 12));
			edgeChild.setGeometry(spk::Rect2D(-2, 4, 4, 4));
			clippedChild.setGeometry(spk::Rect2D(8, 8, 8, 8));
			grandChild.setGeometry(spk::Rect2D(2, 2, 4, 4));
			outsideChild.setGeometry(spk::Rect2D(20, 0, 4, 4));
		}
	};

	void fillRect(
		std::vector<unsigned char>& p_pixels,
		int p_width,
		int p_height,
		const spk::Rect2D& p_rect,
		const std::array<unsigned char, 4>& p_color)
	{
		for (int y = p_rect.top(); y < p_rect.bottom(); ++y)
		{
			for (int x = p_rect.left(); x < p_rect.right(); ++x)
			{
				if (x < 0 || y < 0 || x >= p_width || y >= p_height)
				{
					continue;
				}

				const std::size_t index =
					(static_cast<std::size_t>(y) * static_cast<std::size_t>(p_width) + static_cast<std::size_t>(x)) * 4;
				p_pixels[index + 0] = p_color[0];
				p_pixels[index + 1] = p_color[1];
				p_pixels[index + 2] = p_color[2];
				p_pixels[index + 3] = p_color[3];
			}
		}
	}

	void writeHierarchyExpectedImage(const std::filesystem::path& p_path, int p_width, int p_height)
	{
		std::vector<unsigned char> pixels(static_cast<std::size_t>(p_width) * static_cast<std::size_t>(p_height) * 4, 0);
		for (std::size_t index = 0; index < pixels.size(); index += 4)
		{
			pixels[index + 3] = 255;
		}

		fillRect(pixels, p_width, p_height, spk::Rect2D(2, 2, 12, 12), {255, 0, 0, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(2, 6, 2, 4), {0, 255, 255, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(10, 10, 4, 4), {0, 255, 0, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(12, 12, 2, 2), {255, 255, 0, 255});

		ASSERT_NE(stbi_write_png(p_path.string().c_str(), p_width, p_height, 4, pixels.data(), p_width * 4), 0);
	}

	void writeMovedHierarchyExpectedImage(const std::filesystem::path& p_path, int p_width, int p_height)
	{
		std::vector<unsigned char> pixels(static_cast<std::size_t>(p_width) * static_cast<std::size_t>(p_height) * 4, 0);
		for (std::size_t index = 0; index < pixels.size(); index += 4)
		{
			pixels[index + 3] = 255;
		}

		fillRect(pixels, p_width, p_height, spk::Rect2D(0, 0, 12, 12), {255, 0, 0, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(0, 4, 2, 4), {0, 255, 255, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(8, 8, 4, 4), {0, 255, 0, 255});
		fillRect(pixels, p_width, p_height, spk::Rect2D(10, 10, 2, 2), {255, 255, 0, 255});

		ASSERT_NE(stbi_write_png(p_path.string().c_str(), p_width, p_height, 4, pixels.data(), p_width * 4), 0);
	}
}

TEST(OpenGLWidgetViewportScissorTest, ComputesViewportAndScissorThroughThreeLevelHierarchy)
{
	ViewportScissorHierarchy hierarchy;

	EXPECT_EQ(hierarchy.parent.geometry(), spk::Rect2D(2, 2, 12, 12));
	EXPECT_EQ(hierarchy.parent.absoluteGeometryForTest().anchor, spk::Vector2Int(2, 2));
	EXPECT_EQ(hierarchy.parent.absoluteGeometryForTest(), spk::Rect2D(2, 2, 12, 12));
	EXPECT_EQ(hierarchy.parent.scissorForTest(), spk::Rect2D(2, 2, 12, 12));

	EXPECT_EQ(hierarchy.edgeChild.geometry(), spk::Rect2D(-2, 4, 4, 4));
	EXPECT_EQ(hierarchy.edgeChild.absoluteGeometryForTest().anchor, spk::Vector2Int(0, 6));
	EXPECT_EQ(hierarchy.edgeChild.absoluteGeometryForTest(), spk::Rect2D(0, 6, 4, 4));
	EXPECT_EQ(hierarchy.edgeChild.scissorForTest(), spk::Rect2D(2, 6, 2, 4));

	EXPECT_EQ(hierarchy.clippedChild.geometry(), spk::Rect2D(8, 8, 8, 8));
	EXPECT_EQ(hierarchy.clippedChild.absoluteGeometryForTest().anchor, spk::Vector2Int(10, 10));
	EXPECT_EQ(hierarchy.clippedChild.absoluteGeometryForTest(), spk::Rect2D(10, 10, 8, 8));
	EXPECT_EQ(hierarchy.clippedChild.scissorForTest(), spk::Rect2D(10, 10, 4, 4));

	EXPECT_EQ(hierarchy.grandChild.geometry(), spk::Rect2D(2, 2, 4, 4));
	EXPECT_EQ(hierarchy.grandChild.absoluteGeometryForTest().anchor, spk::Vector2Int(12, 12));
	EXPECT_EQ(hierarchy.grandChild.absoluteGeometryForTest(), spk::Rect2D(12, 12, 4, 4));
	EXPECT_EQ(hierarchy.grandChild.scissorForTest(), spk::Rect2D(12, 12, 2, 2));

	EXPECT_EQ(hierarchy.outsideChild.geometry(), spk::Rect2D(20, 0, 4, 4));
	EXPECT_EQ(hierarchy.outsideChild.absoluteGeometryForTest().anchor, spk::Vector2Int(22, 2));
	EXPECT_EQ(hierarchy.outsideChild.absoluteGeometryForTest(), spk::Rect2D(22, 2, 4, 4));
	EXPECT_EQ(hierarchy.outsideChild.scissorForTest(), spk::Rect2D(22, 2, 0, 0));
}

TEST(OpenGLWidgetViewportScissorTest, RecomputesDescendantScissorsWhenParentGeometryChanges)
{
	ViewportScissorHierarchy hierarchy;

	hierarchy.parent.setGeometry(spk::Rect2D(0, 0, 12, 12));

	EXPECT_EQ(hierarchy.clippedChild.geometry(), spk::Rect2D(8, 8, 8, 8));
	EXPECT_EQ(hierarchy.clippedChild.absoluteGeometryForTest().anchor, spk::Vector2Int(8, 8));
	EXPECT_EQ(hierarchy.clippedChild.absoluteGeometryForTest(), spk::Rect2D(8, 8, 8, 8));
	EXPECT_EQ(hierarchy.clippedChild.scissorForTest(), spk::Rect2D(8, 8, 4, 4));

	EXPECT_EQ(hierarchy.grandChild.geometry(), spk::Rect2D(2, 2, 4, 4));
	EXPECT_EQ(hierarchy.grandChild.absoluteGeometryForTest().anchor, spk::Vector2Int(10, 10));
	EXPECT_EQ(hierarchy.grandChild.absoluteGeometryForTest(), spk::Rect2D(10, 10, 4, 4));
	EXPECT_EQ(hierarchy.grandChild.scissorForTest(), spk::Rect2D(10, 10, 2, 2));
}

TEST(OpenGLWidgetViewportScissorTest, VisualHierarchyIsClippedByParentScissors)
{
	constexpr int width = 20;
	constexpr int height = 20;

	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	// The scissor test is always enabled and its region survives from previous
	// tests: cover the whole target so the clear is complete.
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ViewportScissorHierarchy hierarchy;
	spk::RenderSnapshotBuilder snapshotBuilder;
	hierarchy.parent.appendRenderUnits(snapshotBuilder);
	spk::RenderSnapshot snapshot = snapshotBuilder.build();
	snapshot.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("widget_geometry_scissor_hierarchy_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("widget_geometry_scissor_hierarchy_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("widget_geometry_scissor_hierarchy_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	writeHierarchyExpectedImage(expected, width, height);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches)
		<< "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
	EXPECT_EQ(result.differentPixelCount, 0);
}

TEST(OpenGLWidgetViewportScissorTest, VisualHierarchyUsesUpdatedAbsoluteGeometryAndScissorAfterParentMove)
{
	constexpr int width = 20;
	constexpr int height = 20;

	sparkle_test::OpenGLTestContext context(spk::Rect2D(0, 0, width, height));
	spk::RenderContext& renderContext = context.renderContext();

	// The scissor test is always enabled and its region survives from previous
	// tests: cover the whole target so the clear is complete.
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ViewportScissorHierarchy hierarchy;
	hierarchy.parent.setGeometry(spk::Rect2D(0, 0, 12, 12));

	spk::RenderSnapshotBuilder snapshotBuilder;
	hierarchy.parent.appendRenderUnits(snapshotBuilder);
	spk::RenderSnapshot snapshot = snapshotBuilder.build();
	snapshot.execute(renderContext);
	context.gpuRuntime().waitUntilWorkDone();

	const std::filesystem::path actual = sparkle_test::resultImagePath("widget_moved_geometry_scissor_hierarchy_actual.png");
	const std::filesystem::path expected = sparkle_test::expectedImagePath("widget_moved_geometry_scissor_hierarchy_expected.png");
	const std::filesystem::path diff = sparkle_test::resultImagePath("widget_moved_geometry_scissor_hierarchy_diff.png");
	context.gpuRuntime().saveScreenshot(actual, spk::Rect2D(0, 0, width, height));
	writeMovedHierarchyExpectedImage(expected, width, height);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actual, expected, diff);
	EXPECT_TRUE(result.matches)
		<< "actual=[" << actual.string() << "] expected=[" << expected.string() << "] diff=[" << diff.string() << "]";
	EXPECT_EQ(result.differentPixelCount, 0);
}

