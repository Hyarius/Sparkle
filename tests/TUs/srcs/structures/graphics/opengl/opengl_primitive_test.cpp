#include <gtest/gtest.h>

#include <stdexcept>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_primitive.hpp"

TEST(OpenGLPrimitiveTest, MapsEveryPrimitiveToGlEnum)
{
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::Points), static_cast<GLenum>(GL_POINTS));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::Lines), static_cast<GLenum>(GL_LINES));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::LineLoop), static_cast<GLenum>(GL_LINE_LOOP));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::LineStrip), static_cast<GLenum>(GL_LINE_STRIP));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::Triangles), static_cast<GLenum>(GL_TRIANGLES));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::TriangleStrip), static_cast<GLenum>(GL_TRIANGLE_STRIP));
	EXPECT_EQ(spk::OpenGL::primitiveType(spk::Primitive::TriangleFan), static_cast<GLenum>(GL_TRIANGLE_FAN));
}

TEST(OpenGLPrimitiveTest, ThrowsOnUnsupportedPrimitive)
{
	const auto invalid = static_cast<spk::Primitive>(0xFFFF);
	EXPECT_THROW(auto primitiveType = spk::OpenGL::primitiveType(invalid), std::runtime_error);
}
