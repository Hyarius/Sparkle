#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

#include "spk_binary_field.hpp"

namespace
{
	struct Uint3
	{
		std::uint32_t x;
		std::uint32_t y;
		std::uint32_t z;
	};
}

TEST(BinaryFieldTest, DefaultConstructedFieldIsInvalid)
{
	spk::BinaryField field;

	EXPECT_FALSE(field.isValid());
	EXPECT_THROW(field.name(), std::runtime_error);
	EXPECT_THROW(field.bytes(), std::runtime_error);
}

TEST(BinaryFieldTest, RootFieldWrapsExternalBytes)
{
	std::array<std::uint8_t, 16> data{};
	spk::BinaryField field(data.data(), data.size());

	EXPECT_TRUE(field.isValid());
	EXPECT_EQ(field.name(), "<root>");
	EXPECT_EQ(field.size(), data.size());
	EXPECT_EQ(field.offset(), 0u);
	EXPECT_EQ(field.data(), data.data());
}

TEST(BinaryFieldTest, NullDataWithNonZeroSizeThrows)
{
	EXPECT_THROW(spk::BinaryField(nullptr, 4), std::runtime_error);
}

TEST(BinaryFieldTest, AddsNamedValueAtExplicitOffset)
{
	std::array<std::uint8_t, 16> data{};
	spk::BinaryField root(data.data(), data.size());

	spk::BinaryField value = root.addValue("Value", 4, sizeof(std::uint32_t));

	EXPECT_EQ(value.name(), "Value");
	EXPECT_EQ(value.offset(), 4u);
	EXPECT_EQ(value.size(), sizeof(std::uint32_t));
	EXPECT_EQ(value.data(), data.data() + 4);
}

TEST(BinaryFieldTest, RejectsChildOutsideParentBounds)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());

	EXPECT_THROW(root.addValue("TooLarge", 4, 8), std::runtime_error);
}

TEST(BinaryFieldTest, AddsObjectAndValidatesChildrenAgainstObjectSize)
{
	std::array<std::uint8_t, 16> data{};
	spk::BinaryField root(data.data(), data.size());

	spk::BinaryField object = root.addObject("Object", 4, 4);
	spk::BinaryField value = object.addValue("Byte", 2, sizeof(std::uint8_t));

	EXPECT_EQ(object.data(), data.data() + 4);
	EXPECT_EQ(value.data(), data.data() + 6);
	EXPECT_THROW(object.addValue("Overflow", 3, 2), std::runtime_error);
}

TEST(BinaryFieldTest, NamedLookupReturnsChild)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());

	root.addValue("A", 0, 1);

	EXPECT_EQ(root["A"].data(), data.data());
	EXPECT_THROW(root["Missing"], std::runtime_error);
}

TEST(BinaryFieldTest, DuplicateNamesThrow)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());

	root.addValue("A", 0, 1);

	EXPECT_THROW(root.addValue("A", 1, 1), std::runtime_error);
}

TEST(BinaryFieldTest, AddsFixedArrayAndIndexedElements)
{
	std::array<std::uint8_t, 16> data{};
	spk::BinaryField root(data.data(), data.size());

	spk::BinaryField values = root.addArray("Values", 4, 3, sizeof(std::uint16_t));

	EXPECT_EQ(values.size(), 6u);
	EXPECT_EQ(values.count(), 3u);
	EXPECT_EQ(values.elementSize(), sizeof(std::uint16_t));
	EXPECT_EQ(values[0].data(), data.data() + 4);
	EXPECT_EQ(values[1].data(), data.data() + 6);
	EXPECT_EQ(values[2].data(), data.data() + 8);
	EXPECT_THROW(values[3], std::runtime_error);
}

TEST(BinaryFieldTest, ArrayCannotReceiveNamedChildren)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField values = root.addArray("Values", 0, 2, 1);

	EXPECT_THROW(values.addValue("A", 0, 1), std::runtime_error);
}

TEST(BinaryFieldTest, AssignsTriviallyCopyableScalarWithExactSize)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField value = root.addValue("Value", 2, sizeof(std::uint32_t));

	value = std::uint32_t{0xAABBCCDDu};

	EXPECT_EQ(value.as<std::uint32_t>(), 0xAABBCCDDu);
	EXPECT_THROW(value = std::uint16_t{0xFFFFu}, std::runtime_error);
}

TEST(BinaryFieldTest, AssignsOneByteValueWithExplicitByteType)
{
	std::array<std::uint8_t, 4> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField value = root.addValue("Byte", 1, sizeof(std::uint8_t));

	value = std::uint8_t{0xFFu};

	EXPECT_EQ(data[1], 0xFFu);
}

TEST(BinaryFieldTest, AssignsStdArrayWhenByteSizeMatches)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField values = root.addArray("Values", 2, 3, sizeof(std::uint8_t));

	const std::array<std::uint8_t, 3> source{0xAAu, 0xBBu, 0xCCu};
	values = source;

	EXPECT_EQ(data[2], 0xAAu);
	EXPECT_EQ(data[3], 0xBBu);
	EXPECT_EQ(data[4], 0xCCu);
}

TEST(BinaryFieldTest, IndexedArrayElementAssignmentChecksElementSize)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField values = root.addArray("Values", 0, 2, sizeof(std::uint16_t));

	values[1] = std::uint16_t{0xCAFEu};

	EXPECT_EQ(values[1].as<std::uint16_t>(), 0xCAFEu);
	EXPECT_THROW(values[0] = std::uint8_t{0xFFu}, std::runtime_error);
}

TEST(BinaryFieldTest, ObjectAssignmentCopiesExactObjectBytes)
{
	std::array<std::uint8_t, 16> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField object = root.addObject("Position", 0, sizeof(Uint3));

	const Uint3 source{1u, 2u, 3u};
	object = source;

	EXPECT_EQ(object.as<Uint3>().x, 1u);
	EXPECT_EQ(object.as<Uint3>().y, 2u);
	EXPECT_EQ(object.as<Uint3>().z, 3u);
}

TEST(BinaryFieldTest, BytesExposeTheSelectedSection)
{
	std::array<std::uint8_t, 8> data{};
	spk::BinaryField root(data.data(), data.size());
	spk::BinaryField value = root.addValue("Value", 3, 2);

	std::span<std::uint8_t> bytes = value.bytes();
	ASSERT_EQ(bytes.size(), 2u);
	bytes[0] = 10u;
	bytes[1] = 20u;

	EXPECT_EQ(data[3], 10u);
	EXPECT_EQ(data[4], 20u);
}
