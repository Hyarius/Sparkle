#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"

#if defined(_WIN32) && defined(SPARKLE_GPU_BACKEND_OPENGL)

TEST(OpenGLUniformBufferObjectTest, BindsConfiguredBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::UniformBufferObject uniformBuffer(3, spk::OpenGL::BufferObject::Usage::DynamicDraw, 16);
	uniformBuffer.activate();

	GLint boundBuffer = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 3, &boundBuffer);
	EXPECT_EQ(static_cast<GLuint>(boundBuffer), uniformBuffer.id());
	EXPECT_EQ(uniformBuffer.bindingPoint().value(), 3u);
}

TEST(OpenGLUniformBufferObjectTest, CanClearBindingPoint)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;

	spk::OpenGL::UniformBufferObject uniformBuffer(2, spk::OpenGL::BufferObject::Usage::DynamicDraw, 16);
	uniformBuffer.clearBindingPoint();
	EXPECT_FALSE(uniformBuffer.bindingPoint().has_value());
}

#endif
