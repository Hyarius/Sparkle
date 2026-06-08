#include <gtest/gtest.h>
#include <stdexcept>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"


#include "structures/graphics/opengl/spk_opengl_gpu_data_buffer_center.hpp"

namespace
{
	struct GPUDataBufferCenterFixture : public ::testing::Test
	{
		sparkle_test::OpenGLTestContext context;

		std::shared_ptr<spk::UniformBufferObject> makeUBO(unsigned int p_bindingPoint = 0)
		{
			return std::make_shared<spk::UniformBufferObject>(
				p_bindingPoint,
				spk::BufferObject::Usage::DynamicDraw,
				16);
		}

		std::shared_ptr<spk::ShaderStorageBufferObject> makeSSBO(unsigned int p_bindingPoint = 0)
		{
			return std::make_shared<spk::ShaderStorageBufferObject>(
				p_bindingPoint,
				spk::BufferObject::Usage::DynamicDraw,
				16);
		}

		void TearDown() override
		{
			spk::GPUDataBufferCenter::remove("TestUBO");
			spk::GPUDataBufferCenter::remove("TestSSBO");
			spk::GPUDataBufferCenter::remove("TypeMismatch");
		}
	};
}

TEST_F(GPUDataBufferCenterFixture, AddUBOThrowsOnNullBuffer)
{
	EXPECT_THROW(
		spk::GPUDataBufferCenter::addUBO("TestUBO", nullptr),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, AddSSBOThrowsOnNullBuffer)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	EXPECT_THROW(
		spk::GPUDataBufferCenter::addSSBO("TestSSBO", nullptr),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, AddSSBOStoresAndGetSSBORetrieves)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	auto ssbo = makeSSBO(5);
	spk::GPUDataBufferCenter::addSSBO("TestSSBO", ssbo);

	EXPECT_TRUE(spk::GPUDataBufferCenter::contains("TestSSBO"));
	spk::ShaderStorageBufferObject& retrieved = spk::GPUDataBufferCenter::getSSBO("TestSSBO");
	EXPECT_EQ(retrieved.id(), ssbo->id());
}

TEST_F(GPUDataBufferCenterFixture, GetSSBOThrowsWhenNameNotFound)
{
	EXPECT_THROW(
		spk::GPUDataBufferCenter::getSSBO("TestSSBO"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, GetUBOThrowsOnTypeMismatch)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	auto ssbo = makeSSBO(6);
	spk::GPUDataBufferCenter::addSSBO("TypeMismatch", ssbo);

	EXPECT_THROW(
		spk::GPUDataBufferCenter::getUBO("TypeMismatch"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, GetSSBOThrowsOnTypeMismatch)
{
	auto ubo = makeUBO(7);
	spk::GPUDataBufferCenter::addUBO("TypeMismatch", ubo);

	EXPECT_THROW(
		spk::GPUDataBufferCenter::getSSBO("TypeMismatch"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, RemoveDeletesEntry)
{
	auto ubo = makeUBO(8);
	spk::GPUDataBufferCenter::addUBO("TestUBO", ubo);
	ASSERT_TRUE(spk::GPUDataBufferCenter::contains("TestUBO"));

	spk::GPUDataBufferCenter::remove("TestUBO");

	EXPECT_FALSE(spk::GPUDataBufferCenter::contains("TestUBO"));
}

TEST_F(GPUDataBufferCenterFixture, RemoveNonExistentNameDoesNotThrow)
{
	EXPECT_NO_THROW(spk::GPUDataBufferCenter::remove("TestUBO"));
}

