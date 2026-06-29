#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/spk_shader_storage_buffer_object.hpp"

namespace
{
	struct Record
	{
		std::uint32_t a = 7;
		std::uint32_t b = 11;
	};

	static_assert(std::is_trivially_copyable_v<Record>);

	struct Header
	{
		std::uint32_t count = 0;
		std::uint32_t flags = 0;
		float scale[2] = {1.0f, 1.0f};
	};

	static_assert(std::is_trivially_copyable_v<Header>);
	static_assert(sizeof(Header) == 16);
}


TEST(OpenGLShaderStorageBufferObjectTest, BindsConfiguredBindingPointWhenSupported)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	spk::ShaderStorageBufferObject storageBuffer(4, spk::BufferObject::Usage::DynamicDraw, 16);
	storageBuffer.activate(context.renderContext());

	GLint boundBuffer = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 4, &boundBuffer);
	EXPECT_EQ(static_cast<GLuint>(boundBuffer), storageBuffer.gpu(context.renderContext()).id());
	EXPECT_EQ(storageBuffer.bindingPoint().value(), 4u);
}

TEST(OpenGLShaderStorageBufferObjectTest, CanClearBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(1, spk::BufferObject::Usage::DynamicDraw, 16);
	storageBuffer.clearBindingPoint();
	EXPECT_FALSE(storageBuffer.bindingPoint().has_value());
}

TEST(OpenGLShaderStorageBufferObjectTest, ActivateWithoutBindingPointSkipsBaseBinding)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(spk::BufferObject::Usage::DynamicDraw, 16);
	EXPECT_FALSE(storageBuffer.bindingPoint().has_value());
	EXPECT_NO_THROW(storageBuffer.activate(context.renderContext()));
	EXPECT_NE(storageBuffer.gpu(context.renderContext()).id(), 0u);
}

TEST(OpenGLShaderStorageBufferObjectTest, CountDerivesFromRecordsAfterHeader)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	constexpr std::size_t headerSize = 16;

	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record),
		headerSize);

	EXPECT_EQ(storageBuffer.elementSize(), sizeof(Record));
	EXPECT_EQ(storageBuffer.arrayOffset(), headerSize);
	EXPECT_EQ(storageBuffer.size(), headerSize);
	EXPECT_EQ(storageBuffer.count(), 0u);

	storageBuffer.emplaceBack<Record>();
	storageBuffer.pushBack(Record{1, 2});
	EXPECT_EQ(storageBuffer.count(), 2u);
	EXPECT_EQ(storageBuffer.size(), headerSize + 2u * sizeof(Record));

	storageBuffer.clear();
	EXPECT_EQ(storageBuffer.count(), 0u);
	EXPECT_EQ(storageBuffer.size(), headerSize);
}

TEST(OpenGLShaderStorageBufferObjectTest, FreeFormBufferReportsZeroCount)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(spk::BufferObject::Usage::DynamicDraw, 16);

	storageBuffer.emplaceBack<Record>();
	EXPECT_EQ(storageBuffer.elementSize(), 0u);
	EXPECT_EQ(storageBuffer.count(), 0u);
}

TEST(OpenGLShaderStorageBufferObjectTest, RejectsWritesIncompatibleWithRecordLayout)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record));

	EXPECT_THROW(storageBuffer.emplaceBack<std::uint8_t>(), std::runtime_error);

	std::array<std::uint8_t, 4> partial = {1, 2, 3, 4};
	EXPECT_THROW(storageBuffer.append(partial.data(), partial.size()), std::runtime_error);
	EXPECT_THROW(storageBuffer.resize(sizeof(Record) + 1), std::runtime_error);

	std::array<Record, 2> records = {Record{1, 2}, Record{3, 4}};
	EXPECT_NO_THROW(storageBuffer.append(records.data(), sizeof(records)));
	EXPECT_EQ(storageBuffer.count(), 2u);
	EXPECT_NO_THROW(storageBuffer.resize(sizeof(Record)));
	EXPECT_EQ(storageBuffer.count(), 1u);
}

TEST(OpenGLShaderStorageBufferObjectTest, ConstructionRejectsSizeWithPartialRecord)
{
	EXPECT_THROW(
		spk::ShaderStorageBufferObject(
			spk::BufferObject::Usage::DynamicDraw,
			sizeof(Record) + 1,
			sizeof(Record)),
		std::runtime_error);
}

TEST(OpenGLShaderStorageBufferObjectTest, EmplaceBackConstructsRecordsAndPacksTightly)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record));

	Record &first = storageBuffer.emplaceBack<Record>();
	EXPECT_EQ(first.a, 7u);
	EXPECT_EQ(first.b, 11u);
	first.a = 100;

	Record &second = storageBuffer.emplaceBack<Record>();
	second.a = 200;
	second.b = 201;

	EXPECT_EQ(storageBuffer.count(), 2u);
	EXPECT_EQ(storageBuffer.size(), 2u * sizeof(Record));

	const spk::ShaderStorageBufferObject &readOnly = storageBuffer;
	Record stored0;
	Record stored1;
	std::memcpy(&stored0, readOnly.data(), sizeof(Record));
	std::memcpy(&stored1, readOnly.data() + sizeof(Record), sizeof(Record));

	EXPECT_EQ(stored0.a, 100u);
	EXPECT_EQ(stored0.b, 11u);
	EXPECT_EQ(stored1.a, 200u);
	EXPECT_EQ(stored1.b, 201u);
}

TEST(OpenGLShaderStorageBufferObjectTest, PushBackCopiesValue)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record));

	const Record source = {42, 43};
	Record &stored = storageBuffer.pushBack(source);

	EXPECT_EQ(stored.a, 42u);
	EXPECT_EQ(stored.b, 43u);
	EXPECT_EQ(storageBuffer.count(), 1u);
}

TEST(OpenGLShaderStorageBufferObjectTest, ElementsSpansTheRecordArray)
{
	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record));

	storageBuffer.pushBack(Record{1, 2});
	storageBuffer.pushBack(Record{3, 4});

	std::span<Record> records = storageBuffer.elements<Record>();
	ASSERT_EQ(records.size(), 2u);
	EXPECT_EQ(records[0].a, 1u);
	EXPECT_EQ(records[1].b, 4u);

	records[1].a = 99;
	const spk::ShaderStorageBufferObject &readOnly = storageBuffer;
	std::span<const Record> constRecords = readOnly.elements<Record>();
	ASSERT_EQ(constRecords.size(), 2u);
	EXPECT_EQ(constRecords[1].a, 99u);
}

TEST(OpenGLShaderStorageBufferObjectTest, ElementsThrowsWhenTypeSizeMismatchesRecord)
{
	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		0,
		sizeof(Record));

	EXPECT_THROW({ [[maybe_unused]] auto records = storageBuffer.elements<std::uint8_t>(); }, std::runtime_error);
}

TEST(OpenGLShaderStorageBufferObjectTest, ElementsThrowsWhenBufferHasNoRecordLayout)
{
	spk::ShaderStorageBufferObject storageBuffer(spk::BufferObject::Usage::DynamicDraw, 16);

	EXPECT_THROW({ [[maybe_unused]] auto records = storageBuffer.elements<Record>(); }, std::runtime_error);
}

TEST(OpenGLShaderStorageBufferObjectTest, CastOverlaysHeaderAndRecordArray)
{
	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		sizeof(Header),
		sizeof(Record),
		sizeof(Header));

	storageBuffer.pushBack(Record{10, 20});
	storageBuffer.pushBack(Record{30, 40});

	auto view = storageBuffer.cast<Header, Record>();
	view.fixed.count = 2;
	view.fixed.flags = 1;
	ASSERT_EQ(view.elements.size(), 2u);
	view.elements[0].a = 111;

	const spk::ShaderStorageBufferObject &readOnly = storageBuffer;
	auto constView = readOnly.cast<Header, Record>();
	EXPECT_EQ(constView.fixed.count, 2u);
	EXPECT_EQ(constView.fixed.flags, 1u);
	ASSERT_EQ(constView.elements.size(), 2u);
	EXPECT_EQ(constView.elements[0].a, 111u);
	EXPECT_EQ(constView.elements[1].b, 40u);
}

TEST(OpenGLShaderStorageBufferObjectTest, CastThrowsWhenFixedSizeMismatchesArrayOffset)
{
	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		sizeof(Header),
		sizeof(Record),
		sizeof(Header));

	EXPECT_THROW({ [[maybe_unused]] auto view = (storageBuffer.cast<Record, Record>()); }, std::runtime_error);
}

TEST(OpenGLShaderStorageBufferObjectTest, CastThrowsWhenElementSizeMismatchesRecord)
{
	spk::ShaderStorageBufferObject storageBuffer(
		spk::BufferObject::Usage::DynamicDraw,
		sizeof(Header),
		sizeof(Record),
		sizeof(Header));

	EXPECT_THROW({ [[maybe_unused]] auto view = (storageBuffer.cast<Header, Header>()); }, std::runtime_error);
}

