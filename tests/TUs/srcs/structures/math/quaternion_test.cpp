#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

#include "structures/math/spk_quaternion.hpp"

namespace
{
	void expectQuaternionNear(const spk::Quaternion& p_actual, const spk::Quaternion& p_expected)
	{
		EXPECT_NEAR(p_actual.x, p_expected.x, 0.000001f);
		EXPECT_NEAR(p_actual.y, p_expected.y, 0.000001f);
		EXPECT_NEAR(p_actual.z, p_expected.z, 0.000001f);
		EXPECT_NEAR(p_actual.w, p_expected.w, 0.000001f);
	}
}

TEST(QuaternionTest, DefaultConstructsIdentity)
{
	const spk::Quaternion quaternion;

	EXPECT_FLOAT_EQ(quaternion.x, 0.0f);
	EXPECT_FLOAT_EQ(quaternion.y, 0.0f);
	EXPECT_FLOAT_EQ(quaternion.z, 0.0f);
	EXPECT_FLOAT_EQ(quaternion.w, 1.0f);
	expectQuaternionNear(spk::Quaternion::identity(), quaternion);
}

TEST(QuaternionTest, ConstructsFromComponents)
{
	const spk::Quaternion quaternion(1.0f, 2.0f, 3.0f, 4.0f);

	EXPECT_FLOAT_EQ(quaternion.x, 1.0f);
	EXPECT_FLOAT_EQ(quaternion.y, 2.0f);
	EXPECT_FLOAT_EQ(quaternion.z, 3.0f);
	EXPECT_FLOAT_EQ(quaternion.w, 4.0f);
}

TEST(QuaternionTest, FormatsAsExpected)
{
	const spk::Quaternion quaternion(1.0f, -2.0f, 3.5f, 4.0f);
	std::ostringstream outputStream;
	outputStream << quaternion;

	EXPECT_EQ(quaternion.toString(), "(1, -2, 3.5, 4)");
	EXPECT_EQ(outputStream.str(), "(1, -2, 3.5, 4)");
}

TEST(QuaternionTest, NormalizesComponents)
{
	const spk::Quaternion quaternion(2.0f, 0.0f, 0.0f, 0.0f);
	const spk::Quaternion normalized = quaternion.normalized();

	expectQuaternionNear(normalized, spk::Quaternion(1.0f, 0.0f, 0.0f, 0.0f));
	EXPECT_NEAR(std::sqrt(
					normalized.x * normalized.x +
					normalized.y * normalized.y +
					normalized.z * normalized.z +
					normalized.w * normalized.w),
				1.0f,
				0.000001f);
}

TEST(QuaternionTest, NormalizingZeroLengthThrows)
{
	EXPECT_THROW(static_cast<void>(spk::Quaternion(0.0f, 0.0f, 0.0f, 0.0f).normalized()), std::runtime_error);
}

TEST(QuaternionTest, ConstructsFromAxisAngleInDegrees)
{
	const spk::Quaternion quaternion = spk::Quaternion::fromAxisAngle(spk::Vector3(0.0f, 0.0f, 1.0f), 180.0f);

	EXPECT_NEAR(quaternion.x, 0.0f, 0.000001f);
	EXPECT_NEAR(quaternion.y, 0.0f, 0.000001f);
	EXPECT_NEAR(quaternion.z, 1.0f, 0.000001f);
	EXPECT_NEAR(quaternion.w, 0.0f, 0.000001f);
	EXPECT_THROW(static_cast<void>(spk::Quaternion::fromAxisAngle(spk::Vector3::Zero, 30.0f)), std::runtime_error);
}

TEST(QuaternionTest, ConvertsEulerAnglesRoundTrip)
{
	const spk::Vector3 angles(20.0f, -15.0f, 30.0f);
	const spk::Vector3 converted = spk::Quaternion::fromEuler(angles).toEuler();

	EXPECT_NEAR(converted.x, angles.x, 0.00001f);
	EXPECT_NEAR(converted.y, angles.y, 0.00001f);
	EXPECT_NEAR(converted.z, angles.z, 0.00001f);
}

TEST(QuaternionTest, SlerpInterpolatesAxisRotation)
{
	const spk::Quaternion halfway = spk::Quaternion::slerp(
		spk::Quaternion::identity(),
		spk::Quaternion::fromAxisAngle(spk::Vector3(0.0f, 0.0f, 1.0f), 180.0f),
		0.5f);

	EXPECT_NEAR(std::fabs(halfway.z), std::sqrt(0.5f), 0.000001f);
	EXPECT_NEAR(halfway.w, std::sqrt(0.5f), 0.000001f);
}

TEST(QuaternionTest, LookAtReturnsIdentityForDefaultForwardDirection)
{
	const spk::Quaternion orientation = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(0.0f, 0.0f, -1.0f));

	expectQuaternionNear(orientation, spk::Quaternion::identity());
	EXPECT_THROW(
		static_cast<void>(spk::Quaternion::lookAt(spk::Vector3::Zero, spk::Vector3::Zero)),
		std::runtime_error);
}

TEST(QuaternionTest, LookAtAcceptsDirectionParallelToRequestedUp)
{
	const spk::Quaternion orientation = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(0.0f, 1.0f, 0.0f));

	EXPECT_NEAR(orientation.dot(orientation), 1.0f, 0.000001f);
}

TEST(QuaternionTest, LookAtUsesVerticalFallbackWhenForwardParallelToCustomUp)
{
	// forward == up == +X makes (forward x up) zero, and |forward.y| < 0.999,
	// exercising the vertical-fallback branch of lookAt.
	const spk::Quaternion orientation = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(1.0f, 0.0f, 0.0f),
		spk::Vector3(1.0f, 0.0f, 0.0f));

	EXPECT_NEAR(orientation.dot(orientation), 1.0f, 0.000001f);
}

TEST(QuaternionTest, LookAtSelectsEachLargestDiagonalBranch)
{
	// trace <= 0 with m11 the largest diagonal element (default up).
	const spk::Quaternion yawAround = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(0.0f, 0.0f, 1.0f));
	EXPECT_NEAR(yawAround.dot(yawAround), 1.0f, 0.000001f);

	// trace <= 0 with m00 the largest diagonal element (inverted up).
	const spk::Quaternion rollAround = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(0.0f, 0.0f, 1.0f),
		spk::Vector3(0.0f, -1.0f, 0.0f));
	EXPECT_NEAR(rollAround.dot(rollAround), 1.0f, 0.000001f);

	// trace <= 0 falling through to the m22 branch.
	const spk::Quaternion rolledForward = spk::Quaternion::lookAt(
		spk::Vector3::Zero,
		spk::Vector3(0.0f, 0.0f, -1.0f),
		spk::Vector3(0.0f, -1.0f, 0.0f));
	EXPECT_NEAR(rolledForward.dot(rolledForward), 1.0f, 0.000001f);
}

TEST(QuaternionTest, SlerpCoversShortPathAndLinearFallback)
{
	// Positive dot product (cosine >= 0) keeps the destination unchanged, and a
	// moderate angle exercises the trigonometric interpolation path.
	const spk::Quaternion moderate = spk::Quaternion::slerp(
		spk::Quaternion::identity(),
		spk::Quaternion::fromAxisAngle(spk::Vector3(0.0f, 0.0f, 1.0f), 90.0f),
		0.5f);
	EXPECT_NEAR(moderate.dot(moderate), 1.0f, 0.000001f);

	// Nearly-identical quaternions (cosine > 0.9995) take the linear fallback.
	const spk::Quaternion close = spk::Quaternion::slerp(
		spk::Quaternion::identity(),
		spk::Quaternion::fromAxisAngle(spk::Vector3(0.0f, 0.0f, 1.0f), 0.5f),
		0.5f);
	EXPECT_NEAR(close.dot(close), 1.0f, 0.000001f);
}
