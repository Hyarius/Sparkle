#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <stdexcept>

#include "spk_binary_object.hpp"
#include "spk_binary_field.hpp"

namespace
{
	using namespace spk;

	struct Uint3
	{
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t z;
	};

	struct Uint4
	{
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t z;
		std::uint32_t w;
	};
}

TEST(BinaryObjectTest, DefaultConstructionProducesUsableEmptyRoot)
{
	BinaryObject object;

	BinaryField root = object.root();
	EXPECT_TRUE(root.isValid());
	EXPECT_TRUE(root.isObject());
	EXPECT_EQ(root.name(), "<root>");
	EXPECT_EQ(object.size(), 0u);
	EXPECT_EQ(object.buffer().size(), 0u);
}

TEST(BinaryObjectTest, RootAccessByNameWorks)
{
	BinaryObject object;
	object.addValue("Version", sizeof(std::uint32_t));

	BinaryField field = object["Version"];

	EXPECT_TRUE(field.isValue());
	EXPECT_EQ(field.name(), "Version");
	EXPECT_EQ(field.size(), sizeof(std::uint32_t));
}

TEST(BinaryObjectTest, RootAccessByNameThrowsWhenMissing)
{
	BinaryObject object;

	EXPECT_THROW(object["Missing"], std::runtime_error);
}

TEST(BinaryObjectTest, BufferSizeMatchesComputedObjectSize)
{
	BinaryObject object;
	object.addValue("A", sizeof(std::uint32_t));
	object.addValue("B", sizeof(std::uint32_t));

	EXPECT_EQ(object.buffer().size(), 8);
	EXPECT_EQ(object.buffer().size(), object.size());

	object.addValue("C", sizeof(std::uint32_t));
	
	EXPECT_EQ(object.buffer().size(), 12);
	EXPECT_EQ(object.buffer().size(), object.size());
}

struct Byte1
{
	std::uint8_t value;
};

struct Half1
{
	std::uint16_t value;
};

struct Word1
{
	std::uint32_t value;
};

struct Float2
{
	float values[2];
};

struct Float3
{
	float values[3];
};

struct Float4
{
	float values[4];
};

struct Float11
{
	float values[11];
};

struct Float16
{
	float values[16];
};

struct Float23
{
	float values[23];
};

TEST(BinaryObjectTest, PackedValueLayoutUsesStrictSequentialOffsetsForManySizes)
{
	BinaryObject object(BinaryLayoutMode::Packed);

	BinaryField byteField = object.addValue("Byte", sizeof(Byte1));
	BinaryField halfField = object.addValue("Half", sizeof(Half1));
	BinaryField wordField = object.addValue("Word", sizeof(Word1));
	BinaryField float2Field = object.addValue("Float2", sizeof(Float2));
	BinaryField float3Field = object.addValue("Float3", sizeof(Float3));
	BinaryField float4Field = object.addValue("Float4", sizeof(Float4));
	BinaryField float11Field = object.addValue("Float11", sizeof(Float11));
	BinaryField float16Field = object.addValue("Float16", sizeof(Float16));
	BinaryField float23Field = object.addValue("Float23", sizeof(Float23));
	BinaryField float2ArrayField = object.addArray("Float2Array", 3, sizeof(Float2));

	EXPECT_EQ(byteField.alignment(), 1u);
	EXPECT_EQ(halfField.alignment(), 1u);
	EXPECT_EQ(wordField.alignment(), 1u);
	EXPECT_EQ(float2Field.alignment(), 1u);
	EXPECT_EQ(float3Field.alignment(), 1u);
	EXPECT_EQ(float4Field.alignment(), 1u);
	EXPECT_EQ(float11Field.alignment(), 1u);
	EXPECT_EQ(float16Field.alignment(), 1u);
	EXPECT_EQ(float23Field.alignment(), 1u);
	EXPECT_EQ(float2ArrayField.alignment(), 1u);
	EXPECT_EQ(float2ArrayField.stride(), 8u);
	EXPECT_EQ(float2ArrayField.count(), 3u);
	EXPECT_EQ(float2ArrayField.size(), 24u);

	EXPECT_EQ(byteField.offset(), 0u);
	EXPECT_EQ(halfField.offset(), 1u);
	EXPECT_EQ(wordField.offset(), 3u);
	EXPECT_EQ(float2Field.offset(), 7u);
	EXPECT_EQ(float3Field.offset(), 15u);
	EXPECT_EQ(float4Field.offset(), 27u);
	EXPECT_EQ(float11Field.offset(), 43u);
	EXPECT_EQ(float16Field.offset(), 87u);
	EXPECT_EQ(float23Field.offset(), 151u);
	EXPECT_EQ(float2ArrayField.offset(), 243u);

	EXPECT_EQ(float2ArrayField[0].offset(), 243u);
	EXPECT_EQ(float2ArrayField[1].offset(), 251u);
	EXPECT_EQ(float2ArrayField[2].offset(), 259u);

	EXPECT_EQ(object.size(), 267u);
}

TEST(BinaryObjectTest, ScalarAlignedValueLayoutUsesNaturalAlignmentBucketsForManySizes)
{
	BinaryObject object(BinaryLayoutMode::ScalarAligned);

	BinaryField byteField = object.addValue("Byte", sizeof(Byte1));
	BinaryField halfField = object.addValue("Half", sizeof(Half1));
	BinaryField wordField = object.addValue("Word", sizeof(Word1));
	BinaryField float2Field = object.addValue("Float2", sizeof(Float2));
	BinaryField float3Field = object.addValue("Float3", sizeof(Float3));
	BinaryField float4Field = object.addValue("Float4", sizeof(Float4));
	BinaryField float11Field = object.addValue("Float11", sizeof(Float11));
	BinaryField float16Field = object.addValue("Float16", sizeof(Float16));
	BinaryField float23Field = object.addValue("Float23", sizeof(Float23));
	BinaryField float2ArrayField = object.addArray("Float2Array", 3, sizeof(Float2));

	EXPECT_EQ(byteField.alignment(), 1u);
	EXPECT_EQ(halfField.alignment(), 2u);
	EXPECT_EQ(wordField.alignment(), 4u);
	EXPECT_EQ(float2Field.alignment(), 8u);
	EXPECT_EQ(float3Field.alignment(), 16u);
	EXPECT_EQ(float4Field.alignment(), 16u);
	EXPECT_EQ(float11Field.alignment(), 16u);
	EXPECT_EQ(float16Field.alignment(), 16u);
	EXPECT_EQ(float23Field.alignment(), 16u);

	EXPECT_EQ(float2ArrayField.alignment(), 8u);
	EXPECT_EQ(float2ArrayField.stride(), 8u);
	EXPECT_EQ(float2ArrayField.count(), 3u);
	EXPECT_EQ(float2ArrayField.size(), 24u);

	EXPECT_EQ(byteField.offset(), 0u);
	EXPECT_EQ(halfField.offset(), 2u);
	EXPECT_EQ(wordField.offset(), 4u);
	EXPECT_EQ(float2Field.offset(), 8u);
	EXPECT_EQ(float3Field.offset(), 16u);
	EXPECT_EQ(float4Field.offset(), 32u);
	EXPECT_EQ(float11Field.offset(), 48u);
	EXPECT_EQ(float16Field.offset(), 96u);
	EXPECT_EQ(float23Field.offset(), 160u);
	EXPECT_EQ(float2ArrayField.offset(), 256u);

	EXPECT_EQ(float2ArrayField[0].offset(), 256u);
	EXPECT_EQ(float2ArrayField[1].offset(), 264u);
	EXPECT_EQ(float2ArrayField[2].offset(), 272u);

	EXPECT_EQ(object.size(), 288u);
}

TEST(BinaryObjectTest, Std140ValueLayoutUsesStdRulesForManySizes)
{
	BinaryObject object(BinaryLayoutMode::Std140);

	BinaryField byteField = object.addValue("Byte", sizeof(Byte1));
	BinaryField halfField = object.addValue("Half", sizeof(Half1));
	BinaryField wordField = object.addValue("Word", sizeof(Word1));
	BinaryField float2Field = object.addValue("Float2", sizeof(Float2));
	BinaryField float3Field = object.addValue("Float3", sizeof(Float3));
	BinaryField float4Field = object.addValue("Float4", sizeof(Float4));
	BinaryField float11Field = object.addValue("Float11", sizeof(Float11));
	BinaryField float16Field = object.addValue("Float16", sizeof(Float16));
	BinaryField float23Field = object.addValue("Float23", sizeof(Float23));
	BinaryField float2ArrayField = object.addArray("Float2Array", 3, sizeof(Float2));

	EXPECT_EQ(byteField.alignment(), 4u);
	EXPECT_EQ(halfField.alignment(), 4u);
	EXPECT_EQ(wordField.alignment(), 4u);
	EXPECT_EQ(float2Field.alignment(), 8u);
	EXPECT_EQ(float3Field.alignment(), 16u);
	EXPECT_EQ(float4Field.alignment(), 16u);
	EXPECT_EQ(float11Field.alignment(), 16u);
	EXPECT_EQ(float16Field.alignment(), 16u);
	EXPECT_EQ(float23Field.alignment(), 16u);

	EXPECT_EQ(float2ArrayField.alignment(), 16u);
	EXPECT_EQ(float2ArrayField.stride(), 16u);
	EXPECT_EQ(float2ArrayField.count(), 3u);
	EXPECT_EQ(float2ArrayField.size(), 48u);

	EXPECT_EQ(byteField.offset(), 0u);
	EXPECT_EQ(halfField.offset(), 4u);
	EXPECT_EQ(wordField.offset(), 8u);
	EXPECT_EQ(float2Field.offset(), 16u);
	EXPECT_EQ(float3Field.offset(), 32u);
	EXPECT_EQ(float4Field.offset(), 48u);
	EXPECT_EQ(float11Field.offset(), 64u);
	EXPECT_EQ(float16Field.offset(), 112u);
	EXPECT_EQ(float23Field.offset(), 176u);
	EXPECT_EQ(float2ArrayField.offset(), 272u);

	EXPECT_EQ(float2ArrayField[0].offset(), 272u);
	EXPECT_EQ(float2ArrayField[1].offset(), 288u);
	EXPECT_EQ(float2ArrayField[2].offset(), 304u);

	EXPECT_EQ(object.size(), 320u);
}

TEST(BinaryObjectTest, Std430ValueLayoutUsesStdRulesForManySizes)
{
	BinaryObject object(BinaryLayoutMode::Std430);

	BinaryField byteField = object.addValue("Byte", sizeof(Byte1));
	BinaryField halfField = object.addValue("Half", sizeof(Half1));
	BinaryField wordField = object.addValue("Word", sizeof(Word1));
	BinaryField float2Field = object.addValue("Float2", sizeof(Float2));
	BinaryField float3Field = object.addValue("Float3", sizeof(Float3));
	BinaryField float4Field = object.addValue("Float4", sizeof(Float4));
	BinaryField float11Field = object.addValue("Float11", sizeof(Float11));
	BinaryField float16Field = object.addValue("Float16", sizeof(Float16));
	BinaryField float23Field = object.addValue("Float23", sizeof(Float23));
	BinaryField float2ArrayField = object.addArray("Float2Array", 3, sizeof(Float2));

	EXPECT_EQ(byteField.alignment(), 4u);
	EXPECT_EQ(halfField.alignment(), 4u);
	EXPECT_EQ(wordField.alignment(), 4u);
	EXPECT_EQ(float2Field.alignment(), 8u);
	EXPECT_EQ(float3Field.alignment(), 16u);
	EXPECT_EQ(float4Field.alignment(), 16u);
	EXPECT_EQ(float11Field.alignment(), 16u);
	EXPECT_EQ(float16Field.alignment(), 16u);
	EXPECT_EQ(float23Field.alignment(), 16u);

	EXPECT_EQ(float2ArrayField.alignment(), 8u);
	EXPECT_EQ(float2ArrayField.stride(), 8u);
	EXPECT_EQ(float2ArrayField.count(), 3u);
	EXPECT_EQ(float2ArrayField.size(), 24u);

	EXPECT_EQ(byteField.offset(), 0u);
	EXPECT_EQ(halfField.offset(), 4u);
	EXPECT_EQ(wordField.offset(), 8u);
	EXPECT_EQ(float2Field.offset(), 16u);
	EXPECT_EQ(float3Field.offset(), 32u);
	EXPECT_EQ(float4Field.offset(), 48u);
	EXPECT_EQ(float11Field.offset(), 64u);
	EXPECT_EQ(float16Field.offset(), 112u);
	EXPECT_EQ(float23Field.offset(), 176u);
	EXPECT_EQ(float2ArrayField.offset(), 272u);

	EXPECT_EQ(float2ArrayField[0].offset(), 272u);
	EXPECT_EQ(float2ArrayField[1].offset(), 280u);
	EXPECT_EQ(float2ArrayField[2].offset(), 288u);

	EXPECT_EQ(object.size(), 304u);
}

TEST(BinaryObjectTest, PackedArrayStrideEqualsElementSize)
{
	BinaryObject object(BinaryLayoutMode::Packed);
	BinaryField values = object.addArray("Values", 3, sizeof(Uint3));

	EXPECT_EQ(values.alignment(), 1u);
	EXPECT_EQ(values.stride(), sizeof(Uint3));
	EXPECT_EQ(values.size(), 3u * sizeof(Uint3));

	EXPECT_EQ(values[0].offset(), 0u);
	EXPECT_EQ(values[1].offset(), sizeof(Uint3));
	EXPECT_EQ(values[2].offset(), 2u * sizeof(Uint3));
}

TEST(BinaryObjectTest, ScalarAlignedArrayStrideUsesAlignedElementSize)
{
	BinaryObject object(BinaryLayoutMode::ScalarAligned);
	BinaryField values = object.addArray("Values", 3, sizeof(Uint3));

	EXPECT_EQ(values.alignment(), 16u);
	EXPECT_EQ(values.stride(), 16u);
	EXPECT_EQ(values.size(), 48u);

	EXPECT_EQ(values[0].offset(), 0u);
	EXPECT_EQ(values[1].offset(), 16u);
	EXPECT_EQ(values[2].offset(), 32u);
}

TEST(BinaryObjectTest, Std140ArrayAlwaysUses16ByteAlignmentAndStride)
{
	BinaryObject object(BinaryLayoutMode::Std140);
	BinaryField values = object.addArray("Values", 3, sizeof(std::uint32_t));

	EXPECT_EQ(values.alignment(), 16u);
	EXPECT_EQ(values.stride(), 16u);
	EXPECT_EQ(values.size(), 48u);

	EXPECT_EQ(values[0].offset(), 0u);
	EXPECT_EQ(values[1].offset(), 16u);
	EXPECT_EQ(values[2].offset(), 32u);
}

TEST(BinaryObjectTest, Std430ArrayUsesElementBasedAlignment)
{
	BinaryObject object(BinaryLayoutMode::Std430);
	BinaryField values = object.addArray("Values", 3, sizeof(std::uint32_t));

	EXPECT_EQ(values.alignment(), 4u);
	EXPECT_EQ(values.stride(), 4u);
	EXPECT_EQ(values.size(), 12u);

	EXPECT_EQ(values[0].offset(), 0u);
	EXPECT_EQ(values[1].offset(), 4u);
	EXPECT_EQ(values[2].offset(), 8u);
}

TEST(BinaryObjectTest, NestedPackedObjectHasSequentialOffsets)
{
	BinaryObject object(BinaryLayoutMode::Packed);

	BinaryField transform = object.addObject("Transform");
	BinaryField position = transform.addValue("Position", sizeof(Uint3));
	BinaryField rotation = transform.addValue("Rotation", sizeof(Uint4));

	EXPECT_EQ(transform.offset(), 0u);
	EXPECT_EQ(position.offset(), 0u);
	EXPECT_EQ(rotation.offset(), sizeof(Uint3));
	EXPECT_EQ(transform.size(), sizeof(Uint3) + sizeof(Uint4));
	EXPECT_EQ(object.size(), transform.size());
}

TEST(BinaryObjectTest, NestedScalarAlignedObjectUsesMaximumChildAlignment)
{
	BinaryObject object(BinaryLayoutMode::ScalarAligned);

	BinaryField transform = object.addObject("Transform");
	BinaryField position = transform.addValue("Position", sizeof(Uint3));
	BinaryField rotation = transform.addValue("Rotation", sizeof(std::uint32_t));

	EXPECT_EQ(transform.alignment(), 16u);
	EXPECT_EQ(transform.offset(), 0u);

	EXPECT_EQ(position.offset(), 0u);
	EXPECT_EQ(position.alignment(), 16u);

	EXPECT_EQ(rotation.offset(), 12u);
	EXPECT_EQ(rotation.alignment(), 4u);

	EXPECT_EQ(transform.size(), 16u);
	EXPECT_EQ(object.size(), 16u);
}

TEST(BinaryObjectTest, NestedStd140ObjectUses16ByteObjectAlignment)
{
	BinaryObject object(BinaryLayoutMode::Std140);

	BinaryField outer = object.addObject("Outer");
	BinaryField a = outer.addValue("A", sizeof(std::uint32_t));
	BinaryField b = outer.addValue("B", sizeof(std::uint32_t));

	EXPECT_EQ(outer.alignment(), 16u);
	EXPECT_EQ(outer.offset(), 0u);

	EXPECT_EQ(a.offset(), 0u);
	EXPECT_EQ(b.offset(), 4u);

	EXPECT_EQ(outer.size(), 16u);
	EXPECT_EQ(object.size(), 16u);
}

TEST(BinaryObjectTest, NestedStd430ObjectUsesMaximumChildAlignment)
{
	BinaryObject object(BinaryLayoutMode::Std430);

	BinaryField outer = object.addObject("Outer");
	BinaryField a = outer.addValue("A", sizeof(Uint3));
	BinaryField b = outer.addValue("B", sizeof(std::uint32_t));

	EXPECT_EQ(outer.alignment(), 16u);
	EXPECT_EQ(outer.offset(), 0u);

	EXPECT_EQ(a.offset(), 0u);
	EXPECT_EQ(b.offset(), 12u);

	EXPECT_EQ(outer.size(), 16u);
	EXPECT_EQ(object.size(), 16u);
}

TEST(BinaryObjectTest, WritingThroughFieldsUpdatesUnderlyingBuffer)
{
	BinaryObject object(BinaryLayoutMode::Packed);

	BinaryField a = object.addValue("A", sizeof(std::uint32_t));
	BinaryField b = object.addValue("B", sizeof(std::uint32_t));

	a.set<std::uint32_t>(0x11223344u);
	b.set<std::uint32_t>(0x55667788u);

	const std::span<std::uint8_t> raw = object.buffer();
	ASSERT_EQ(raw.size(), 8u);

	EXPECT_EQ(a.as<std::uint32_t>(), 0x11223344u);
	EXPECT_EQ(b.as<std::uint32_t>(), 0x55667788u);
}

TEST(BinaryObjectTest, ResizingArrayUpdatesObjectSize)
{
	BinaryObject object(BinaryLayoutMode::Packed);
	BinaryField values = object.addArray("Values", 2, sizeof(std::uint32_t));

	EXPECT_EQ(object.size(), 8u);

	values.resize(5);
	EXPECT_EQ(object.size(), 20u);

	values.resize(1);
	EXPECT_EQ(object.size(), 4u);
}

TEST(BinaryObjectTest, InsertingNewFieldAfterExistingSynchronizedDataKeepsExistingFieldReadable)
{
	BinaryObject object(BinaryLayoutMode::Packed);

	object.addValue("Version", sizeof(std::uint32_t));
	BinaryField position = object.addValue("Position", sizeof(Uint3));

	position.set<Uint3>({1u, 2u, 3u});

	const std::size_t oldPositionOffset = position.offset();
	EXPECT_EQ(oldPositionOffset, sizeof(std::uint32_t));

	object.root().addArray("Values", 2, sizeof(std::uint32_t));

	const std::size_t newPositionOffset = object["Position"].offset();

	EXPECT_EQ(newPositionOffset, oldPositionOffset);

	const Uint3 readback = object["Position"].as<Uint3>();
	EXPECT_EQ(readback.x, 1u);
	EXPECT_EQ(readback.y, 2u);
	EXPECT_EQ(readback.z, 3u);
}