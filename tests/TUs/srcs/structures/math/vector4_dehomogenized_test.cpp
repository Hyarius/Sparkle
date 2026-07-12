#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/math/spk_vector4.hpp"

TEST(Vector4DehomogenizedTest, DividesXYZByW)
{
	const spk::Vector4 homogeneous{2.0f, 4.0f, 6.0f, 2.0f};

	EXPECT_EQ(homogeneous.dehomogenized(), spk::Vector3(1.0f, 2.0f, 3.0f));
}

TEST(Vector4DehomogenizedTest, ReturnsFloatingPointResultForIntegralVector)
{
	const spk::Vector4Int homogeneous{3, 6, 9, 3};

	EXPECT_EQ(homogeneous.dehomogenized(), spk::Vector3(1.0f, 2.0f, 3.0f));
}

TEST(Vector4DehomogenizedTest, RejectsZeroW)
{
	const spk::Vector4 homogeneous{1.0f, 2.0f, 3.0f, 0.0f};

	EXPECT_THROW((void)homogeneous.dehomogenized(), std::invalid_argument);
}
