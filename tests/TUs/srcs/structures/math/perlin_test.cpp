#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

#include "structures/math/spk_perlin.hpp"

namespace
{
	constexpr float NOT_A_NUMBER = std::numeric_limits<float>::quiet_NaN();
	constexpr float INFINITY_VALUE = std::numeric_limits<float>::infinity();
}

TEST(PerlinTest, DefaultConstructorUsesDefaultParameters)
{
	const spk::Perlin perlin;

	const spk::Perlin::Parameters &parameters = perlin.parameters();

	EXPECT_EQ(parameters.seed, 0u);
	EXPECT_EQ(parameters.octaves, 3);
	EXPECT_FLOAT_EQ(parameters.persistence, 0.5f);
	EXPECT_FLOAT_EQ(parameters.lacunarity, 2.0f);
	EXPECT_FLOAT_EQ(parameters.frequency, 1.0f);
	EXPECT_EQ(parameters.interpolation, spk::Perlin::Interpolation::SmootherStep);
}

TEST(PerlinTest, ParameterConstructorStoresParameters)
{
	spk::Perlin::Parameters parameters;
	parameters.seed = 1234;
	parameters.octaves = 5;
	parameters.persistence = 0.25f;
	parameters.lacunarity = 3.0f;
	parameters.frequency = 0.5f;
	parameters.interpolation = spk::Perlin::Interpolation::Linear;

	const spk::Perlin perlin(parameters);

	EXPECT_EQ(perlin.parameters().seed, 1234u);
	EXPECT_EQ(perlin.parameters().octaves, 5);
	EXPECT_FLOAT_EQ(perlin.parameters().persistence, 0.25f);
	EXPECT_EQ(perlin.parameters().interpolation, spk::Perlin::Interpolation::Linear);
}

TEST(PerlinTest, SetParametersRejectsNonPositiveOctaves)
{
	spk::Perlin perlin;

	spk::Perlin::Parameters parameters;
	parameters.octaves = 0;

	EXPECT_THROW(perlin.setParameters(parameters), std::invalid_argument);
}

TEST(PerlinTest, SetParametersRejectsInvalidPersistence)
{
	spk::Perlin perlin;

	spk::Perlin::Parameters negative;
	negative.persistence = -0.1f;
	EXPECT_THROW(perlin.setParameters(negative), std::invalid_argument);

	spk::Perlin::Parameters notFinite;
	notFinite.persistence = NOT_A_NUMBER;
	EXPECT_THROW(perlin.setParameters(notFinite), std::invalid_argument);
}

TEST(PerlinTest, SetParametersRejectsInvalidLacunarity)
{
	spk::Perlin perlin;

	spk::Perlin::Parameters zero;
	zero.lacunarity = 0.0f;
	EXPECT_THROW(perlin.setParameters(zero), std::invalid_argument);

	spk::Perlin::Parameters notFinite;
	notFinite.lacunarity = INFINITY_VALUE;
	EXPECT_THROW(perlin.setParameters(notFinite), std::invalid_argument);
}

TEST(PerlinTest, SetParametersRejectsInvalidFrequency)
{
	spk::Perlin perlin;

	spk::Perlin::Parameters zero;
	zero.frequency = 0.0f;
	EXPECT_THROW(perlin.setParameters(zero), std::invalid_argument);

	spk::Perlin::Parameters notFinite;
	notFinite.frequency = NOT_A_NUMBER;
	EXPECT_THROW(perlin.setParameters(notFinite), std::invalid_argument);
}

TEST(PerlinTest, ConstructorWithInvalidParametersThrows)
{
	spk::Perlin::Parameters parameters;
	parameters.octaves = -1;

	EXPECT_THROW(spk::Perlin{parameters}, std::invalid_argument);
}

TEST(PerlinTest, ReseedChangesStoredSeed)
{
	spk::Perlin perlin;

	perlin.reseed(99);

	EXPECT_EQ(perlin.parameters().seed, 99u);
}

TEST(PerlinTest, Raw1DRejectsNonFiniteInput)
{
	const spk::Perlin perlin;

	EXPECT_THROW(auto value = perlin.raw1D(NOT_A_NUMBER), std::invalid_argument);
}

TEST(PerlinTest, Raw2DRejectsNonFiniteInput)
{
	const spk::Perlin perlin;

	EXPECT_THROW(auto value = perlin.raw2D(NOT_A_NUMBER, 0.0f), std::invalid_argument);
	EXPECT_THROW(auto value = perlin.raw2D(0.0f, INFINITY_VALUE), std::invalid_argument);
}

TEST(PerlinTest, Raw3DRejectsNonFiniteInput)
{
	const spk::Perlin perlin;

	EXPECT_THROW(auto value = perlin.raw3D(NOT_A_NUMBER, 0.0f, 0.0f), std::invalid_argument);
	EXPECT_THROW(auto value = perlin.raw3D(0.0f, NOT_A_NUMBER, 0.0f), std::invalid_argument);
	EXPECT_THROW(auto value = perlin.raw3D(0.0f, 0.0f, NOT_A_NUMBER), std::invalid_argument);
}

TEST(PerlinTest, SampleRejectsNonFiniteRange)
{
	const spk::Perlin perlin;

	EXPECT_THROW(auto value = perlin.sample1D(0.5f, NOT_A_NUMBER, 1.0f), std::invalid_argument);
	EXPECT_THROW(auto value = perlin.sample2D(0.5f, 0.5f, 0.0f, NOT_A_NUMBER), std::invalid_argument);
	EXPECT_THROW(auto value = perlin.sample3D(0.5f, 0.5f, 0.5f, NOT_A_NUMBER, 1.0f), std::invalid_argument);
}

TEST(PerlinTest, RawValuesStayWithinNormalizedBounds)
{
	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 7});

	for (int i = -40; i <= 40; ++i)
	{
		const float coordinate = static_cast<float>(i) * 0.37f;

		EXPECT_GE(perlin.raw1D(coordinate), -1.0001f);
		EXPECT_LE(perlin.raw1D(coordinate), 1.0001f);
	}
}

TEST(PerlinTest, SampleMapsIntoRequestedRange)
{
	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 3});

	for (int x = 0; x < 20; ++x)
	{
		for (int y = 0; y < 20; ++y)
		{
			const float value = perlin.sample2D(
				static_cast<float>(x) * 0.21f,
				static_cast<float>(y) * 0.19f,
				-5.0f,
				5.0f);

			EXPECT_GE(value, -5.0f);
			EXPECT_LE(value, 5.0f);
		}
	}
}

TEST(PerlinTest, SameSeedProducesDeterministicOutput)
{
	const spk::Perlin first(spk::Perlin::Parameters{.seed = 555});
	const spk::Perlin second(spk::Perlin::Parameters{.seed = 555});

	EXPECT_FLOAT_EQ(first.raw2D(1.5f, 2.5f), second.raw2D(1.5f, 2.5f));
	EXPECT_FLOAT_EQ(first.raw3D(1.5f, 2.5f, 3.5f), second.raw3D(1.5f, 2.5f, 3.5f));
}

TEST(PerlinTest, DifferentSeedsProduceDifferentOutput)
{
	const spk::Perlin first(spk::Perlin::Parameters{.seed = 1});
	const spk::Perlin second(spk::Perlin::Parameters{.seed = 2});

	EXPECT_NE(first.raw2D(0.5f, 0.5f), second.raw2D(0.5f, 0.5f));
}

TEST(PerlinTest, NoiseIsZeroAtIntegerLatticePoints)
{
	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 12, .octaves = 1});

	// Perlin gradient noise evaluates to zero at integer lattice coordinates.
	EXPECT_NEAR(perlin.raw1D(3.0f), 0.0f, 1e-4f);
	EXPECT_NEAR(perlin.raw2D(2.0f, 4.0f), 0.0f, 1e-4f);
	EXPECT_NEAR(perlin.raw3D(1.0f, 2.0f, 3.0f), 0.0f, 1e-4f);
}

TEST(PerlinTest, NegativeCoordinatesAreHandled)
{
	const spk::Perlin perlin(spk::Perlin::Parameters{.seed = 8});

	// Exercises the floor of negative, non-integer coordinates.
	const float value = perlin.raw3D(-1.4f, -2.6f, -0.5f);

	EXPECT_TRUE(std::isfinite(value));
}

class PerlinInterpolationTest : public ::testing::TestWithParam<spk::Perlin::Interpolation>
{
};

TEST_P(PerlinInterpolationTest, AllInterpolationModesProduceFiniteNoise)
{
	spk::Perlin::Parameters parameters;
	parameters.seed = 21;
	parameters.octaves = 2;
	parameters.interpolation = GetParam();

	const spk::Perlin perlin(parameters);

	for (int x = 0; x < 24; ++x)
	{
		for (int y = 0; y < 24; ++y)
		{
			const float fx = static_cast<float>(x) * 0.33f;
			const float fy = static_cast<float>(y) * 0.27f;

			EXPECT_TRUE(std::isfinite(perlin.raw1D(fx)));
			EXPECT_TRUE(std::isfinite(perlin.raw2D(fx, fy)));
			EXPECT_TRUE(std::isfinite(perlin.raw3D(fx, fy, fx + fy)));
		}
	}
}

INSTANTIATE_TEST_SUITE_P(
	AllModes,
	PerlinInterpolationTest,
	::testing::Values(
		spk::Perlin::Interpolation::Linear,
		spk::Perlin::Interpolation::SmoothStep,
		spk::Perlin::Interpolation::SmootherStep));
