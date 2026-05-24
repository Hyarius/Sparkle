#include <gtest/gtest.h>
#include <stdexcept>

#include "opengl_wrapper_test_utils.hpp"


#include "opengl/spk_opengl_layout_buffer_object.hpp"

using namespace spk;
using LBO = spk::LayoutBufferObject;
using Attr = spk::LayoutBufferObject::Attribute;

namespace
{
	struct LBOFixture : public ::testing::Test
	{
		sparkle_test::OpenGLTestContext context;
	};
}

TEST_F(LBOFixture, DefaultConstructorCreatesEmptyLayout)
{
	LBO lbo;
	EXPECT_EQ(lbo.vertexSize(), 0u);
	EXPECT_EQ(lbo.vertexCount(), 0u);
	EXPECT_FALSE(lbo.isIndexed());
}

TEST_F(LBOFixture, SpanConstructorAddsAttributes)
{
	const std::array<Attr, 2> attrs = {
		Attr{.index = 0, .type = Attr::Type::Vector2, .normalized = false},
		Attr{.index = 1, .type = Attr::Type::Vector3, .normalized = false}
	};
	LBO lbo((std::span<const Attr>(attrs)));

	EXPECT_TRUE(lbo.hasAttribute(0));
	EXPECT_TRUE(lbo.hasAttribute(1));
	const std::size_t expectedSize = 2 * sizeof(float) + 3 * sizeof(float);
	EXPECT_EQ(lbo.vertexSize(), expectedSize);
}

TEST_F(LBOFixture, InitializerListConstructorAddsAttributes)
{
	LBO lbo({
		Attr{.index = 0, .type = Attr::Type::Float, .normalized = false},
		Attr{.index = 1, .type = Attr::Type::Vector2, .normalized = false}
	});

	EXPECT_TRUE(lbo.hasAttribute(0));
	EXPECT_TRUE(lbo.hasAttribute(1));
	EXPECT_EQ(lbo.vertexSize(), sizeof(float) + 2 * sizeof(float));
}

TEST_F(LBOFixture, VertexSizeReflectsAddedAttributes)
{
	LBO lbo;
	EXPECT_EQ(lbo.vertexSize(), 0u);

	lbo.addAttribute(0, Attr::Type::Vector2, false);
	EXPECT_EQ(lbo.vertexSize(), 2 * sizeof(float));

	lbo.addAttribute(1, Attr::Type::Vector3, false);
	EXPECT_EQ(lbo.vertexSize(), 5 * sizeof(float));
}

TEST_F(LBOFixture, AddAttributeThrowsOnDuplicateIndex)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Float, false);

	EXPECT_THROW(lbo.addAttribute(0, Attr::Type::Vector2, false), std::runtime_error);
}

TEST_F(LBOFixture, ClearAttributesResetsLayoutToEmpty)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector3, false);
	lbo.addAttribute(1, Attr::Type::Vector2, false);
	ASSERT_GT(lbo.vertexSize(), 0u);

	lbo.clearAttributes();

	EXPECT_EQ(lbo.vertexSize(), 0u);
	EXPECT_FALSE(lbo.hasAttribute(0));
	EXPECT_FALSE(lbo.hasAttribute(1));
}

TEST_F(LBOFixture, SetVertexBytesThrowsWhenNoAttributesDefined)
{
	LBO lbo;
	const float data[] = {1.0f, 2.0f};

	EXPECT_THROW(lbo.setVertexBytes(data, sizeof(data)), std::runtime_error);
}

TEST_F(LBOFixture, SetVertexBytesThrowsWhenSizeNotAlignedToVertexSize)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	const std::size_t vertexSize = lbo.vertexSize();
	const std::size_t misalignedSize = vertexSize + 1;
	std::vector<std::uint8_t> data(misalignedSize, 0);

	EXPECT_THROW(lbo.setVertexBytes(data.data(), misalignedSize), std::runtime_error);
}

TEST_F(LBOFixture, SetVertexBytesWithEmptySizeSucceedsAndSetsZeroCount)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	EXPECT_NO_THROW(lbo.setVertexBytes(nullptr, 0));
	EXPECT_EQ(lbo.vertexCount(), 0u);
}

TEST_F(LBOFixture, SetVertexBytesAcceptsAlignedData)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	const std::size_t vertexSize = lbo.vertexSize();
	const std::size_t numVertices = 3;
	std::vector<float> data(numVertices * 2, 0.0f);

	EXPECT_NO_THROW(lbo.setVertexBytes(data.data(), numVertices * vertexSize));
	EXPECT_EQ(lbo.vertexCount(), numVertices);
}

TEST_F(LBOFixture, IntAttributeComponentTypeIsGLInt)
{
	EXPECT_EQ(Attr::componentType(Attr::Type::Int), GL_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector2Int), GL_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector3Int), GL_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector4Int), GL_INT);
}

TEST_F(LBOFixture, UIntAttributeComponentTypeIsGLUnsignedInt)
{
	EXPECT_EQ(Attr::componentType(Attr::Type::UInt), GL_UNSIGNED_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector2UInt), GL_UNSIGNED_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector3UInt), GL_UNSIGNED_INT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector4UInt), GL_UNSIGNED_INT);
}

TEST_F(LBOFixture, ScalarIntAndUIntHaveComponentCountOfOne)
{
	EXPECT_EQ(Attr::componentCount(Attr::Type::Int), 1);
	EXPECT_EQ(Attr::componentCount(Attr::Type::UInt), 1);
}

TEST_F(LBOFixture, Vector4TypesHaveComponentCountOfFour)
{
	EXPECT_EQ(Attr::componentCount(Attr::Type::Vector4), 4);
	EXPECT_EQ(Attr::componentCount(Attr::Type::Vector4Int), 4);
	EXPECT_EQ(Attr::componentCount(Attr::Type::Vector4UInt), 4);
}

TEST_F(LBOFixture, FloatAttributeComponentTypeIsGLFloat)
{
	EXPECT_EQ(Attr::componentType(Attr::Type::Float), GL_FLOAT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector2), GL_FLOAT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector3), GL_FLOAT);
	EXPECT_EQ(Attr::componentType(Attr::Type::Vector4), GL_FLOAT);
}

TEST_F(LBOFixture, SetVerticesWithNonEmptySpanUploadsVertexCount)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	struct V2 { float x, y; };
	const std::array<V2, 4> verts = {V2{0,0}, V2{1,0}, V2{1,1}, V2{0,1}};

	lbo.setVertices(std::span<const V2>(verts));

	EXPECT_EQ(lbo.vertexCount(), 4u);
}

TEST_F(LBOFixture, SetVerticesWithEmptySpanSetsZeroCount)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	struct V2 { float x, y; };
	lbo.setVertices(std::span<const V2>{});

	EXPECT_EQ(lbo.vertexCount(), 0u);
}

TEST_F(LBOFixture, SetIndexesUpdatesIndexCountAndIsIndexed)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	const std::array<std::uint32_t, 6> indexes = {0, 1, 2, 0, 2, 3};
	lbo.setIndexes(std::span<const std::uint32_t>(indexes));

	EXPECT_EQ(lbo.indexCount(), 6u);
	EXPECT_TRUE(lbo.isIndexed());
}

TEST_F(LBOFixture, SetIndexesWithEmptySpanClearsIndexed)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	const std::array<std::uint32_t, 3> indexes = {0, 1, 2};
	lbo.setIndexes(std::span<const std::uint32_t>(indexes));
	ASSERT_TRUE(lbo.isIndexed());

	lbo.setIndexes(std::span<const std::uint32_t>{});
	EXPECT_EQ(lbo.indexCount(), 0u);
	EXPECT_FALSE(lbo.isIndexed());
}

TEST_F(LBOFixture, ActivateAndDeactivateDoNotThrow)
{
	LBO lbo;
	lbo.addAttribute(0, Attr::Type::Vector2, false);

	EXPECT_NO_THROW(lbo.activate());
	EXPECT_NO_THROW(lbo.deactivate());
}

