#include <gtest/gtest.h>

#include "opengl_wrapper_test_utils.hpp"
#include "opengl/spk_opengl_uniform.hpp"
#include "opengl/spk_opengl_program.hpp"


namespace
{
	std::shared_ptr<spk::Program> makeUniformProgram()
	{
		return std::make_shared<spk::Program>(
			"#version 330 core\n"
			"void main()\n"
			"{\n"
			"	gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
			"}\n",
			"#version 330 core\n"
			"uniform float uFloat;\n"
			"uniform bool uBool;\n"
			"uniform int uInt;\n"
			"uniform uint uUInt;\n"
			"uniform vec2 uVec2;\n"
			"uniform vec3 uVec3;\n"
			"uniform vec4 uVec4;\n"
			"uniform ivec2 uIVec2;\n"
			"uniform ivec3 uIVec3;\n"
			"uniform ivec4 uIVec4;\n"
			"uniform uvec2 uUVec2;\n"
			"uniform uvec3 uUVec3;\n"
			"uniform uvec4 uUVec4;\n"
			"uniform mat2 uMat2;\n"
			"uniform mat3 uMat3;\n"
			"uniform mat4 uMat4;\n"
			"uniform float aFloat[2];\n"
			"uniform bool aBool[2];\n"
			"uniform int aInt[2];\n"
			"uniform uint aUInt[2];\n"
			"uniform vec2 aVec2[2];\n"
			"uniform vec3 aVec3[2];\n"
			"uniform vec4 aVec4[2];\n"
			"uniform ivec2 aIVec2[2];\n"
			"uniform ivec3 aIVec3[2];\n"
			"uniform ivec4 aIVec4[2];\n"
			"uniform uvec2 aUVec2[2];\n"
			"uniform uvec3 aUVec3[2];\n"
			"uniform uvec4 aUVec4[2];\n"
			"uniform mat2 aMat2[2];\n"
			"uniform mat3 aMat3[2];\n"
			"uniform mat4 aMat4[2];\n"
			"out vec4 outColor;\n"
			"void main()\n"
			"{\n"
			"	float acc = uFloat + float(uBool) + float(uInt) + float(uUInt);\n"
			"	acc += uVec2.x + uVec3.y + uVec4.z;\n"
			"	acc += float(uIVec2.x + uIVec3.y + uIVec4.z);\n"
			"	acc += float(uUVec2.x + uUVec3.y + uUVec4.z);\n"
			"	acc += uMat2[0][0] + uMat3[1][1] + uMat4[2][2];\n"
			"	acc += aFloat[0] + float(aBool[0]) + float(aInt[0]) + float(aUInt[0]);\n"
			"	acc += aVec2[0].x + aVec3[0].y + aVec4[0].z;\n"
			"	acc += float(aIVec2[0].x + aIVec3[0].y + aIVec4[0].z);\n"
			"	acc += float(aUVec2[0].x + aUVec3[0].y + aUVec4[0].z);\n"
			"	acc += aMat2[0][0][0] + aMat3[0][1][1] + aMat4[0][2][2];\n"
			"	outColor = vec4(acc / 1000.0, 0.0, 0.0, 1.0);\n"
			"}\n");
	}
}

TEST(OpenGLUniformTest, ScalarUniformsActivateAndStoreValues)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;
	const auto program = makeUniformProgram();
	program->activate();

	spk::FloatUniform uFloat("uFloat", *program);
	spk::BoolUniform uBool("uBool", *program);
	spk::IntUniform uInt("uInt", *program);
	spk::UIntUniform uUInt("uUInt", *program);
	spk::Vector2Uniform uVec2("uVec2", *program);
	spk::Vector3Uniform uVec3("uVec3", *program);
	spk::Vector4Uniform uVec4("uVec4", *program);
	spk::Vector2IntUniform uIVec2("uIVec2", *program);
	spk::Vector3IntUniform uIVec3("uIVec3", *program);
	spk::Vector4IntUniform uIVec4("uIVec4", *program);
	spk::Vector2UIntUniform uUVec2("uUVec2", *program);
	spk::Vector3UIntUniform uUVec3("uUVec3", *program);
	spk::Vector4UIntUniform uUVec4("uUVec4", *program);
	spk::Matrix2x2Uniform uMat2("uMat2", *program);
	spk::Matrix3x3Uniform uMat3("uMat3", *program);
	spk::Matrix4x4Uniform uMat4("uMat4", *program);

	uFloat = 1.25f;
	uBool = true;
	uInt = -3;
	uUInt = 7u;
	uVec2 = {1.0f, 2.0f};
	uVec3 = {1.0f, 2.0f, 3.0f};
	uVec4 = {1.0f, 2.0f, 3.0f, 4.0f};
	uIVec2 = {1, 2};
	uIVec3 = {1, 2, 3};
	uIVec4 = {1, 2, 3, 4};
	uUVec2 = {1u, 2u};
	uUVec3 = {1u, 2u, 3u};
	uUVec4 = {1u, 2u, 3u, 4u};
	uMat2 = {1.0f, 0.0f, 0.0f, 1.0f};
	uMat3 = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	uMat4 = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

	uFloat.activate();
	uBool.activate();
	uInt.activate();
	uUInt.activate();
	uVec2.activate();
	uVec3.activate();
	uVec4.activate();
	uIVec2.activate();
	uIVec3.activate();
	uIVec4.activate();
	uUVec2.activate();
	uUVec3.activate();
	uUVec4.activate();
	uMat2.activate();
	uMat3.activate();
	uMat4.activate();
	uFloat.forceActivation();

	EXPECT_FALSE(uFloat.needsActivation());
	EXPECT_FLOAT_EQ(uFloat.value(), 1.25f);
}

TEST(OpenGLUniformTest, ArrayUniformsActivateFromVectorAndInitializerList)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;
	const auto program = makeUniformProgram();
	program->activate();

	spk::FloatArrayUniform aFloat("aFloat", *program, 2);
	spk::BoolArrayUniform aBool("aBool", *program, 2);
	spk::IntArrayUniform aInt("aInt", *program, 2);
	spk::UIntArrayUniform aUInt("aUInt", *program, 2);
	spk::Vector2ArrayUniform aVec2("aVec2", *program, 2);
	spk::Vector3ArrayUniform aVec3("aVec3", *program, 2);
	spk::Vector4ArrayUniform aVec4("aVec4", *program, 2);
	spk::Vector2IntArrayUniform aIVec2("aIVec2", *program, 2);
	spk::Vector3IntArrayUniform aIVec3("aIVec3", *program, 2);
	spk::Vector4IntArrayUniform aIVec4("aIVec4", *program, 2);
	spk::Vector2UIntArrayUniform aUVec2("aUVec2", *program, 2);
	spk::Vector3UIntArrayUniform aUVec3("aUVec3", *program, 2);
	spk::Vector4UIntArrayUniform aUVec4("aUVec4", *program, 2);
	spk::Matrix2x2ArrayUniform aMat2("aMat2", *program, 2);
	spk::Matrix3x3ArrayUniform aMat3("aMat3", *program, 2);
	spk::Matrix4x4ArrayUniform aMat4("aMat4", *program, 2);

	aFloat = {1.0f, 2.0f};
	aBool = {true, false};
	aInt = {-1, 2};
	aUInt = {1u, 2u};
	aVec2 = {std::array<float, 2>{1.0f, 2.0f}, std::array<float, 2>{3.0f, 4.0f}};
	aVec3 = {std::array<float, 3>{1.0f, 2.0f, 3.0f}, std::array<float, 3>{4.0f, 5.0f, 6.0f}};
	aVec4 = {std::array<float, 4>{1.0f, 2.0f, 3.0f, 4.0f}, std::array<float, 4>{5.0f, 6.0f, 7.0f, 8.0f}};
	aIVec2 = {std::array<GLint, 2>{1, 2}, std::array<GLint, 2>{3, 4}};
	aIVec3 = {std::array<GLint, 3>{1, 2, 3}, std::array<GLint, 3>{4, 5, 6}};
	aIVec4 = {std::array<GLint, 4>{1, 2, 3, 4}, std::array<GLint, 4>{5, 6, 7, 8}};
	aUVec2 = {std::array<GLuint, 2>{1u, 2u}, std::array<GLuint, 2>{3u, 4u}};
	aUVec3 = {std::array<GLuint, 3>{1u, 2u, 3u}, std::array<GLuint, 3>{4u, 5u, 6u}};
	aUVec4 = {std::array<GLuint, 4>{1u, 2u, 3u, 4u}, std::array<GLuint, 4>{5u, 6u, 7u, 8u}};
	aMat2 = {std::array<float, 4>{1.0f, 0.0f, 0.0f, 1.0f}, std::array<float, 4>{2.0f, 0.0f, 0.0f, 2.0f}};
	aMat3 = {
		std::array<float, 9>{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		std::array<float, 9>{2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 2.0f}};
	aMat4 = {
		std::array<float, 16>{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
		std::array<float, 16>{2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f}};

	aFloat.activate();
	aBool.activate();
	aInt.activate();
	aUInt.activate();
	aVec2.activate();
	aVec3.activate();
	aVec4.activate();
	aIVec2.activate();
	aIVec3.activate();
	aIVec4.activate();
	aUVec2.activate();
	aUVec3.activate();
	aUVec4.activate();
	aMat2.activate();
	aMat3.activate();
	aMat4.activate();

	EXPECT_FALSE(aFloat.needsActivation());
}

TEST(OpenGLUniformTest, ValidationErrorsAreReported)
{
	sparkle_test::OpenGLTestContext context;
	(void)context;
	const auto program = makeUniformProgram();
	program->activate();

	{
		spk::FloatUniform missing("missingUniform", *program);
		EXPECT_THROW(missing.activate(), std::runtime_error);
	}

	EXPECT_THROW(spk::FloatArrayUniform zeroCount("aFloat", *program, 0), std::runtime_error);

	spk::FloatArrayUniform aFloat("aFloat[0]", *program, 2);
	EXPECT_THROW(aFloat.set(nullptr, 2), std::runtime_error);
	EXPECT_THROW(aFloat.set(std::vector<float>{1.0f}), std::runtime_error);
	aFloat = {1.0f, 2.0f};
	EXPECT_NO_THROW(aFloat.activate());

	spk::FloatUniform wrongType("uInt", *program);
	EXPECT_THROW(wrongType.activate(), std::runtime_error);

	spk::FloatArrayUniform tooLarge("aFloat", *program, 3);
	EXPECT_THROW(tooLarge.activate(), std::runtime_error);
}

