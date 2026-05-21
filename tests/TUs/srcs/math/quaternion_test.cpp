#include <gtest/gtest.h>

#include <cmath>
#include <sstream>

#include "math/spk_quaternion.hpp"

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
