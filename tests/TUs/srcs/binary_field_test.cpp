#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <stdexcept>

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

TEST(BinaryFieldTest, DefaultConstructedFieldIsInvalid)
{
	BinaryField field;

	EXPECT_FALSE(field.isValid());

	EXPECT_THROW(field.kind(), std::runtime_error);
	EXPECT_THROW(field.name(), std::runtime_error);
	EXPECT_THROW(field.size(), std::runtime_error);
	EXPECT_THROW(field.offset(), std::runtime_error);
	EXPECT_THROW(field.alignment(), std::runtime_error);
	EXPECT_THROW(field.count(), std::runtime_error);
	EXPECT_THROW(field.stride(), std::runtime_error);
	EXPECT_THROW(field.bytes(), std::runtime_error);
}

TEST(BinaryFieldTest, RootFieldIsValidAndIsObject)
{
	BinaryObject object;
	BinaryField root = object.root();

	EXPECT_TRUE(root.isValid());
	EXPECT_TRUE(root.isObject());
	EXPECT_FALSE(root.isValue());
	EXPECT_FALSE(root.isArray());

	EXPECT_EQ(root.kind(), BinaryFieldKind::Object);
	EXPECT_EQ(root.name(), "<root>");
}

TEST(BinaryFieldTest, AddValueCreatesValueFieldWithExpectedName)
{
	BinaryObject object;
	BinaryField field = object.addValue("Version", sizeof(std::uint32_t));

	EXPECT_TRUE(field.isValid());
	EXPECT_TRUE(field.isValue());
	EXPECT_EQ(field.kind(), BinaryFieldKind::Value);
	EXPECT_EQ(field.name(), "Version");
	EXPECT_EQ(field.size(), sizeof(std::uint32_t));
}

TEST(BinaryFieldTest, AddObjectCreatesObjectField)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Transform");

	EXPECT_TRUE(nested.isObject());
	EXPECT_EQ(nested.name(), "Transform");
}

TEST(BinaryFieldTest, AddArrayCreatesArrayField)
{
	BinaryObject object;
	BinaryField arrayField = object.addArray("Values", 3, sizeof(std::uint32_t));

	EXPECT_TRUE(arrayField.isArray());
	EXPECT_EQ(arrayField.kind(), BinaryFieldKind::Array);
	EXPECT_EQ(arrayField.name(), "Values");
	EXPECT_EQ(arrayField.count(), 3u);
	EXPECT_EQ(arrayField.stride(), sizeof(std::uint32_t));
}

TEST(BinaryFieldTest, AddChildValueInsideObjectWorks)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Transform");
	BinaryField position = nested.addValue("Position", sizeof(Uint3));

	EXPECT_TRUE(position.isValue());
	EXPECT_EQ(position.name(), "Position");
	EXPECT_EQ(position.size(), sizeof(Uint3));

	BinaryField fetched = nested["Position"];
	EXPECT_EQ(fetched.name(), "Position");
	EXPECT_EQ(fetched.size(), sizeof(Uint3));
}

TEST(BinaryFieldTest, AddChildArrayInsideObjectWorks)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Data");
	BinaryField values = nested.addArray("Values", 4, sizeof(std::uint32_t));

	EXPECT_TRUE(values.isArray());
	EXPECT_EQ(values.name(), "Values");
	EXPECT_EQ(values.count(), 4u);

	for (std::size_t index = 0; index < 4; ++index)
	{
		EXPECT_TRUE(values[index].isValue());
		EXPECT_EQ(values[index].size(), sizeof(std::uint32_t));
	}
}

TEST(BinaryFieldTest, AddChildObjectInsideObjectWorks)
{
	BinaryObject object;
	BinaryField root = object.root();
	BinaryField nested = root.addObject("Nested");
	BinaryField child = nested.addObject("SubNested");

	EXPECT_TRUE(nested.isObject());
	EXPECT_TRUE(child.isObject());
	EXPECT_EQ(nested.name(), "Nested");
	EXPECT_EQ(child.name(), "SubNested");
}

TEST(BinaryFieldTest, AccessByNameFromNonObjectThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value["Child"], std::runtime_error);
}

TEST(BinaryFieldTest, AccessByIndexFromNonArrayThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value[0], std::runtime_error);
}

TEST(BinaryFieldTest, AccessMissingChildByNameThrows)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Nested");

	EXPECT_THROW(nested["Missing"], std::runtime_error);
}

TEST(BinaryFieldTest, AccessArrayOutOfRangeThrows)
{
	BinaryObject object;
	BinaryField values = object.addArray("Values", 2, sizeof(std::uint32_t));

	EXPECT_THROW(values[2], std::runtime_error);
	EXPECT_THROW(values[99], std::runtime_error);
}

TEST(BinaryFieldTest, AddValueOnNonObjectThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value.addValue("Other", sizeof(std::uint32_t)), std::runtime_error);
}

TEST(BinaryFieldTest, AddArrayOnNonObjectThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value.addArray("Other", 3, sizeof(std::uint32_t)), std::runtime_error);
}

TEST(BinaryFieldTest, AddObjectOnNonObjectThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value.addObject("Other"), std::runtime_error);
}

TEST(BinaryFieldTest, DuplicateNameInsideSameObjectThrows)
{
	BinaryObject object;
	object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(object.addValue("Value", sizeof(std::uint32_t)), std::runtime_error);
}

TEST(BinaryFieldTest, DuplicateNameInsideNestedObjectThrows)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Nested");
	nested.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(nested.addValue("Value", sizeof(std::uint32_t)), std::runtime_error);
}

TEST(BinaryFieldTest, EmptyNameThrows)
{
	BinaryObject object;
	BinaryField root = object.root();

	EXPECT_THROW(root.addValue("", sizeof(std::uint32_t)), std::runtime_error);
	EXPECT_THROW(root.addArray("", 2, sizeof(std::uint32_t)), std::runtime_error);
	EXPECT_THROW(root.addObject(""), std::runtime_error);
}

TEST(BinaryFieldTest, ZeroSizedValueThrows)
{
	BinaryObject object;

	EXPECT_THROW(object.addValue("Zero", 0), std::runtime_error);
	EXPECT_THROW(object.root().addValue("Zero", 0), std::runtime_error);
}

TEST(BinaryFieldTest, ZeroSizedArrayElementThrows)
{
	BinaryObject object;

	EXPECT_THROW(object.addArray("Array", 3, 0), std::runtime_error);
	EXPECT_THROW(object.root().addArray("Array", 3, 0), std::runtime_error);
}

TEST(BinaryFieldTest, SetAndAsRoundTripScalarWorks)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	value.set<std::uint32_t>(123456u);

	EXPECT_EQ(value.as<std::uint32_t>(), 123456u);
}

TEST(BinaryFieldTest, SetAndAsRoundTripCustomStructureWorks)
{
	BinaryObject object;
	BinaryField position = object.addValue("Position", sizeof(Uint3));

	const Uint3 source{10u, 20u, 30u};
	position.set<Uint3>(source);

	const Uint3 readback = position.as<Uint3>();

	EXPECT_EQ(readback.x, 10u);
	EXPECT_EQ(readback.y, 20u);
	EXPECT_EQ(readback.z, 30u);
}

TEST(BinaryFieldTest, SetWithWrongSizeThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value.set<Uint3>({1u, 2u, 3u}), std::runtime_error);
}

TEST(BinaryFieldTest, AsWithWrongSizeThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));
	value.set<std::uint32_t>(77u);

	EXPECT_THROW(value.as<Uint3>(), std::runtime_error);
}

TEST(BinaryFieldTest, SetOnNonValueThrows)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Nested");

	EXPECT_THROW(nested.set<std::uint32_t>(5u), std::runtime_error);
}

TEST(BinaryFieldTest, AsOnNonValueThrows)
{
	BinaryObject object;
	BinaryField nested = object.addObject("Nested");

	EXPECT_THROW(nested.as<std::uint32_t>(), std::runtime_error);
}

TEST(BinaryFieldTest, BytesExposeWrittenMemoryForValueField)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(Uint4));

	const Uint4 source{1u, 2u, 3u, 4u};
	value.set<Uint4>(source);

	std::span<std::uint8_t> bytes = value.bytes();
	ASSERT_EQ(bytes.size(), sizeof(Uint4));

	const Uint4* typedView = reinterpret_cast<const Uint4*>(bytes.data());
	EXPECT_EQ(typedView->x, 1u);
	EXPECT_EQ(typedView->y, 2u);
	EXPECT_EQ(typedView->z, 3u);
	EXPECT_EQ(typedView->w, 4u);
}

TEST(BinaryFieldTest, ConstBytesExposeWrittenMemoryForValueField)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(Uint4));

	const Uint4 source{5u, 6u, 7u, 8u};
	value.set<Uint4>(source);

	const BinaryField& constField = value;
	std::span<const std::uint8_t> bytes = constField.bytes();
	ASSERT_EQ(bytes.size(), sizeof(Uint4));

	const Uint4* typedView = reinterpret_cast<const Uint4*>(bytes.data());
	EXPECT_EQ(typedView->x, 5u);
	EXPECT_EQ(typedView->y, 6u);
	EXPECT_EQ(typedView->z, 7u);
	EXPECT_EQ(typedView->w, 8u);
}

TEST(BinaryFieldTest, ResizeArrayGrowAddsElements)
{
	BinaryObject object;
	BinaryField values = object.addArray("Values", 2, sizeof(std::uint32_t));

	EXPECT_EQ(values.count(), 2u);

	values.resize(5);

	EXPECT_EQ(values.count(), 5u);
	for (std::size_t index = 0; index < 5; ++index)
	{
		EXPECT_TRUE(values[index].isValue());
		EXPECT_EQ(values[index].size(), sizeof(std::uint32_t));
	}
}

TEST(BinaryFieldTest, ResizeArrayShrinkRemovesElements)
{
	BinaryObject object;
	BinaryField values = object.addArray("Values", 5, sizeof(std::uint32_t));

	values.resize(2);

	EXPECT_EQ(values.count(), 2u);
	EXPECT_THROW(values[2], std::runtime_error);
}

TEST(BinaryFieldTest, ResizeArrayToSameCountKeepsCountStable)
{
	BinaryObject object;
	BinaryField values = object.addArray("Values", 3, sizeof(std::uint32_t));

	values.resize(3);

	EXPECT_EQ(values.count(), 3u);
}

TEST(BinaryFieldTest, ResizeOnNonArrayThrows)
{
	BinaryObject object;
	BinaryField value = object.addValue("Value", sizeof(std::uint32_t));

	EXPECT_THROW(value.resize(10), std::runtime_error);
}

TEST(BinaryFieldTest, ArrayElementsCanStoreIndependentValues)
{
	BinaryObject object;
	BinaryField values = object.addArray("Values", 3, sizeof(std::uint32_t));

	values[0].set<std::uint32_t>(10u);
	values[1].set<std::uint32_t>(20u);
	values[2].set<std::uint32_t>(30u);

	EXPECT_EQ(values[0].as<std::uint32_t>(), 10u);
	EXPECT_EQ(values[1].as<std::uint32_t>(), 20u);
	EXPECT_EQ(values[2].as<std::uint32_t>(), 30u);
}

TEST(BinaryFieldTest, ArrayElementOffsetsFollowStride)
{
	BinaryObject object(BinaryLayoutMode::Packed);
	BinaryField values = object.addArray("Values", 3, sizeof(std::uint32_t));

	EXPECT_EQ(values[1].offset() - values[0].offset(), values.stride());
	EXPECT_EQ(values[2].offset() - values[1].offset(), values.stride());
}

TEST(BinaryFieldTest, NestedFieldLookupsWorkThroughMultipleLevels)
{
	BinaryObject object;
	BinaryField transform = object.addObject("Transform");
	BinaryField rotation = transform.addObject("Rotation");
	BinaryField axis = rotation.addValue("Axis", sizeof(Uint3));

	const Uint3 source{9u, 8u, 7u};
	axis.set<Uint3>(source);

	const Uint3 readback = object["Transform"]["Rotation"]["Axis"].as<Uint3>();

	EXPECT_EQ(readback.x, 9u);
	EXPECT_EQ(readback.y, 8u);
	EXPECT_EQ(readback.z, 7u);
}