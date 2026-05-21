#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLShaderStorageBufferObjectTest, BindsConfiguredBindingPointWhenSupported)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	if (GLEW_VERSION_4_3 == false && GLEW_ARB_shader_storage_buffer_object == false)
	{
		GTEST_SKIP() << "Shader storage buffer objects are not supported by this OpenGL context";
	}

	spk::OpenGL::ShaderStorageBufferObject storageBuffer(4, spk::OpenGL::BufferObject::Usage::DynamicDraw, 16);
	storageBuffer.activate();

	GLint boundBuffer = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 4, &boundBuffer);
	EXPECT_EQ(static_cast<GLuint>(boundBuffer), storageBuffer.id());
	EXPECT_EQ(storageBuffer.bindingPoint().value(), 4u);
}

TEST(OpenGLShaderStorageBufferObjectTest, CanClearBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::ShaderStorageBufferObject storageBuffer(1, spk::OpenGL::BufferObject::Usage::DynamicDraw, 16);
	storageBuffer.clearBindingPoint();
	EXPECT_FALSE(storageBuffer.bindingPoint().has_value());
}

#endif
