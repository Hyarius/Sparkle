#include <gtest/gtest.h>
#include <stdexcept>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"

namespace
{
	struct GPUDataBufferCenterFixture : public ::testing::Test
	{
		sparkle_test::OpenGLTestContext context;

		std::shared_ptr<spk::OpenGL::UniformBufferObject> makeUBO(unsigned int p_bindingPoint = 0)
		{
			return std::make_shared<spk::OpenGL::UniformBufferObject>(
				p_bindingPoint,
				spk::OpenGL::BufferObject::Usage::DynamicDraw,
				16);
		}

		std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject> makeSSBO(unsigned int p_bindingPoint = 0)
		{
			return std::make_shared<spk::OpenGL::ShaderStorageBufferObject>(
				p_bindingPoint,
				spk::OpenGL::BufferObject::Usage::DynamicDraw,
				16);
		}

		void TearDown() override
		{
			spk::OpenGL::GPUDataBufferCenter::remove("TestUBO");
			spk::OpenGL::GPUDataBufferCenter::remove("TestSSBO");
			spk::OpenGL::GPUDataBufferCenter::remove("TypeMismatch");
		}
	};
}

TEST_F(GPUDataBufferCenterFixture, AddUBOThrowsOnNullBuffer)
{
	EXPECT_THROW(
		spk::OpenGL::GPUDataBufferCenter::addUBO("TestUBO", nullptr),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, AddSSBOThrowsOnNullBuffer)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	EXPECT_THROW(
		spk::OpenGL::GPUDataBufferCenter::addSSBO("TestSSBO", nullptr),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, AddSSBOStoresAndGetSSBORetrieves)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	auto ssbo = makeSSBO(5);
	spk::OpenGL::GPUDataBufferCenter::addSSBO("TestSSBO", ssbo);

	EXPECT_TRUE(spk::OpenGL::GPUDataBufferCenter::contains("TestSSBO"));
	spk::OpenGL::ShaderStorageBufferObject& retrieved = spk::OpenGL::GPUDataBufferCenter::getSSBO("TestSSBO");
	EXPECT_EQ(retrieved.id(), ssbo->id());
}

TEST_F(GPUDataBufferCenterFixture, GetSSBOThrowsWhenNameNotFound)
{
	EXPECT_THROW(
		spk::OpenGL::GPUDataBufferCenter::getSSBO("TestSSBO"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, GetUBOThrowsOnTypeMismatch)
{
	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	auto ssbo = makeSSBO(6);
	spk::OpenGL::GPUDataBufferCenter::addSSBO("TypeMismatch", ssbo);

	EXPECT_THROW(
		spk::OpenGL::GPUDataBufferCenter::getUBO("TypeMismatch"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, GetSSBOThrowsOnTypeMismatch)
{
	auto ubo = makeUBO(7);
	spk::OpenGL::GPUDataBufferCenter::addUBO("TypeMismatch", ubo);

	EXPECT_THROW(
		spk::OpenGL::GPUDataBufferCenter::getSSBO("TypeMismatch"),
		std::runtime_error);
}

TEST_F(GPUDataBufferCenterFixture, RemoveDeletesEntry)
{
	auto ubo = makeUBO(8);
	spk::OpenGL::GPUDataBufferCenter::addUBO("TestUBO", ubo);
	ASSERT_TRUE(spk::OpenGL::GPUDataBufferCenter::contains("TestUBO"));

	spk::OpenGL::GPUDataBufferCenter::remove("TestUBO");

	EXPECT_FALSE(spk::OpenGL::GPUDataBufferCenter::contains("TestUBO"));
}

TEST_F(GPUDataBufferCenterFixture, RemoveNonExistentNameDoesNotThrow)
{
	EXPECT_NO_THROW(spk::OpenGL::GPUDataBufferCenter::remove("TestUBO"));
}

#endif
