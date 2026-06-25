#include <gtest/gtest.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"


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

	// No binding point configured: activate() must upload the buffer but skip
	// glBindBufferBase (the has_value() == false branch).
	spk::ShaderStorageBufferObject storageBuffer(spk::BufferObject::Usage::DynamicDraw, 16);
	EXPECT_FALSE(storageBuffer.bindingPoint().has_value());
	EXPECT_NO_THROW(storageBuffer.activate(context.renderContext()));
	EXPECT_NE(storageBuffer.gpu(context.renderContext()).id(), 0u);
}

