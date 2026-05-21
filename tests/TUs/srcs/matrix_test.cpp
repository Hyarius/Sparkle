#include <gtest/gtest.h>

#include "math/spk_matrix.hpp"

namespace
{
	void expectNear(float p_actual, float p_expected)
	{
		EXPECT_NEAR(p_actual, p_expected, 0.0001f);
	}

	void expectVectorNear(const spk::Vector2& p_actual, const spk::Vector2& p_expected)
	{
		expectNear(p_actual.x, p_expected.x);
		expectNear(p_actual.y, p_expected.y);
	}

	void expectVectorNear(const spk::Vector3& p_actual, const spk::Vector3& p_expected)
	{
		expectNear(p_actual.x, p_expected.x);
		expectNear(p_actual.y, p_expected.y);
		expectNear(p_actual.z, p_expected.z);
	}
}

TEST(MatrixTest, DefaultConstructsIdentity)
{
	const spk::Matrix3x3 matrix;

	EXPECT_EQ(matrix, spk::Matrix3x3::identity());
	EXPECT_FLOAT_EQ(matrix[0][0], 1.0f);
	EXPECT_FLOAT_EQ(matrix[1][1], 1.0f);
	EXPECT_FLOAT_EQ(matrix[2][2], 1.0f);
	EXPECT_FLOAT_EQ(matrix[1][0], 0.0f);
}

TEST(MatrixTest, InitializerListUsesRowMajorInputIntoColumnStorage)
{
	const spk::Matrix3x3 matrix{
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f,
		7.0f, 8.0f, 9.0f
	};

	EXPECT_FLOAT_EQ(matrix[0][0], 1.0f);
	EXPECT_FLOAT_EQ(matrix[1][0], 2.0f);
	EXPECT_FLOAT_EQ(matrix[2][0], 3.0f);
	EXPECT_FLOAT_EQ(matrix[0][1], 4.0f);
	EXPECT_THROW((spk::Matrix3x3{1.0f, 2.0f}), std::invalid_argument);
}

TEST(MatrixTest, BoundsChecksColumnAndRowAccess)
{
	spk::Matrix2x2 matrix;

	EXPECT_THROW(matrix[2], std::invalid_argument);
	EXPECT_THROW(matrix[0][2], std::invalid_argument);
}

TEST(MatrixTest, MultipliesMatrix3ByVector2UsingHomogeneousCoordinate)
{
	const spk::Matrix3x3 transform{
		2.0f, 0.0f, 10.0f,
		0.0f, 3.0f, 20.0f,
		0.0f, 0.0f, 1.0f
	};

	expectVectorNear(transform * spk::Vector2(4.0f, 5.0f), spk::Vector2(18.0f, 35.0f));
}

TEST(MatrixTest, MultipliesMatrix4ByVector3)
{
	const spk::Matrix4x4 transform = spk::Matrix4x4::translation(10.0f, 20.0f, 30.0f) * spk::Matrix4x4::scale(2.0f, 3.0f, 4.0f);

	expectVectorNear(transform * spk::Vector3(1.0f, 2.0f, 3.0f), spk::Vector3(12.0f, 26.0f, 42.0f));
}

TEST(MatrixTest, RotationAroundZUsesDegrees)
{
	const spk::Matrix4x4 rotation = spk::Matrix4x4::rotation(0.0f, 0.0f, 90.0f);

	expectVectorNear(rotation * spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 1.0f, 0.0f));
}

TEST(MatrixTest, QuaternionIdentityRotationLeavesVectorUnchanged)
{
	const spk::Matrix4x4 rotation = spk::Matrix4x4::rotation(spk::Quaternion::identity());

	expectVectorNear(rotation * spk::Vector3(1.0f, 2.0f, 3.0f), spk::Vector3(1.0f, 2.0f, 3.0f));
}

TEST(MatrixTest, AxisRotationWorks)
{
	const spk::Matrix4x4 rotation = spk::Matrix4x4::rotateAroundAxis(spk::Vector3(0.0f, 0.0f, 1.0f), 90.0f);

	expectVectorNear(rotation * spk::Vector3(1.0f, 0.0f, 0.0f), spk::Vector3(0.0f, -1.0f, 0.0f));
}

TEST(MatrixTest, OrthoMapsBoundsToClipSpace)
{
	const spk::Matrix4x4 projection = spk::Matrix4x4::ortho(0.0f, 100.0f, 100.0f, 0.0f, -1.0f, 1.0f);

	expectVectorNear(projection * spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(-1.0f, 1.0f, 0.0f));
	expectVectorNear(projection * spk::Vector3(100.0f, 100.0f, 0.0f), spk::Vector3(1.0f, -1.0f, 0.0f));
}

TEST(MatrixTest, PerspectiveHasExpectedProjectionTerms)
{
	const spk::Matrix4x4 projection = spk::Matrix4x4::perspective(90.0f, 2.0f, 1.0f, 11.0f);

	expectNear(projection[0][0], 0.5f);
	expectNear(projection[1][1], 1.0f);
	expectNear(projection[2][2], -1.2f);
	EXPECT_FLOAT_EQ(projection[2][3], -1.0f);
	expectNear(projection[3][2], -2.2f);
}

TEST(MatrixTest, LookAtMovesCameraOrigin)
{
	const spk::Matrix4x4 view = spk::Matrix4x4::lookAt(
		spk::Vector3(0.0f, 0.0f, 5.0f),
		spk::Vector3(0.0f, 0.0f, 0.0f),
		spk::Vector3(0.0f, 1.0f, 0.0f));

	expectVectorNear(view * spk::Vector3(0.0f, 0.0f, 0.0f), spk::Vector3(0.0f, 0.0f, -5.0f));
}

TEST(MatrixTest, DeterminantInvertibilityAndInverseWork)
{
	const spk::Matrix3x3 matrix{
		2.0f, 0.0f, 0.0f,
		0.0f, 3.0f, 0.0f,
		0.0f, 0.0f, 4.0f
	};

	expectNear(matrix.determinant(), 24.0f);
	EXPECT_TRUE(matrix.isInvertible());
	EXPECT_EQ(matrix * matrix.inverse(), spk::Matrix3x3::identity());
}

TEST(MatrixTest, InverseThrowsForSingularMatrix)
{
	const spk::Matrix3x3 matrix{
		1.0f, 2.0f, 3.0f,
		1.0f, 2.0f, 3.0f,
		4.0f, 5.0f, 6.0f
	};

	EXPECT_FLOAT_EQ(matrix.determinant(), 0.0f);
	EXPECT_FALSE(matrix.isInvertible());
	EXPECT_THROW((void)matrix.inverse(), std::runtime_error);
}

TEST(MatrixTest, StringifiesRows)
{
	const spk::Matrix2x2 matrix{
		1.0f, 2.0f,
		3.0f, 4.0f
	};

	EXPECT_EQ(matrix.toString(), "[1 - 2] - [3 - 4]");
}
