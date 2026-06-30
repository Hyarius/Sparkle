#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "structures/container/spk_grid_2d.hpp"
#include "structures/math/spk_poisson_disk.hpp"

namespace
{
	using Point = spk::PoissonDisk::Point;

	float minimumDistance(const std::vector<Point> &p_points)
	{
		float minimum = std::numeric_limits<float>::infinity();

		for (std::size_t i = 0; i < p_points.size(); ++i)
		{
			for (std::size_t j = i + 1; j < p_points.size(); ++j)
			{
				const float dx = p_points[i].x - p_points[j].x;
				const float dy = p_points[i].y - p_points[j].y;

				minimum = std::min(minimum, std::sqrt(dx * dx + dy * dy));
			}
		}

		return minimum;
	}
}

TEST(PoissonDiskTest, GenerateProducesSpacedPointsInsideTheArea)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {40.0f, 30.0f};
	parameters.radius = 3.0f;
	parameters.seed = 17;

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	ASSERT_FALSE(points.empty());
	EXPECT_GT(points.size(), 1u);

	for (const Point &point : points)
	{
		EXPECT_GE(point.x, 0.0f);
		EXPECT_GE(point.y, 0.0f);
		EXPECT_LT(point.x, parameters.size.x);
		EXPECT_LT(point.y, parameters.size.y);
	}

	EXPECT_GE(minimumDistance(points), parameters.radius - 1e-3f);
}

TEST(PoissonDiskTest, GenerateIsDeterministicForAGivenSeed)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {25.0f, 25.0f};
	parameters.radius = 2.5f;
	parameters.seed = 99;

	const std::vector<Point> first = spk::PoissonDisk::generate(parameters);
	const std::vector<Point> second = spk::PoissonDisk::generate(parameters);

	ASSERT_EQ(first.size(), second.size());

	for (std::size_t i = 0; i < first.size(); ++i)
	{
		EXPECT_FLOAT_EQ(first[i].x, second[i].x);
		EXPECT_FLOAT_EQ(first[i].y, second[i].y);
	}
}

TEST(PoissonDiskTest, GenerateRespectsMaxPointCount)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {40.0f, 40.0f};
	parameters.radius = 2.0f;
	parameters.maxPointCount = 5;
	parameters.seed = 4;

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	EXPECT_LE(points.size(), 5u);
	EXPECT_FALSE(points.empty());
}

TEST(PoissonDiskTest, GenerateStopsAtSinglePointWhenMaxCountIsOne)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {40.0f, 40.0f};
	parameters.radius = 2.0f;
	parameters.maxPointCount = 1;
	parameters.seed = 4;

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	EXPECT_EQ(points.size(), 1u);
}

TEST(PoissonDiskTest, GenerateReturnsEmptyWhenNoSeedPointIsAccepted)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {10.0f, 10.0f};
	parameters.radius = 1.0f;
	parameters.seed = 1;
	parameters.accept = [](const Point &) { return false; };

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	EXPECT_TRUE(points.empty());
}

TEST(PoissonDiskTest, GenerateHonoursAcceptPredicate)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {40.0f, 40.0f};
	parameters.radius = 2.0f;
	parameters.seed = 11;
	// Only accept the left half of the area.
	parameters.accept = [](const Point &p_point) { return p_point.x < 20.0f; };

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	ASSERT_FALSE(points.empty());

	for (const Point &point : points)
	{
		EXPECT_LT(point.x, 20.0f);
	}
}

TEST(PoissonDiskTest, GenerateRejectsInvalidSize)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {0.0f, 10.0f};
	parameters.radius = 1.0f;
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);

	parameters.size = {10.0f, -1.0f};
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);

	parameters.size = {std::numeric_limits<float>::quiet_NaN(), 10.0f};
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);

	parameters.size = {10.0f, std::numeric_limits<float>::quiet_NaN()};
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);
}

TEST(PoissonDiskTest, GenerateRejectsInvalidRadius)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {10.0f, 10.0f};
	parameters.radius = 0.0f;
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);

	parameters.radius = std::numeric_limits<float>::infinity();
	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);
}

TEST(PoissonDiskTest, GenerateRejectsInvalidTriesPerPoint)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {10.0f, 10.0f};
	parameters.radius = 1.0f;
	parameters.triesPerPoint = 0;

	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::invalid_argument);
}

TEST(PoissonDiskTest, GenerateThrowsWhenInternalGridOverflows)
{
	spk::PoissonDisk::Parameters parameters;
	parameters.size = {5.0e9f, 5.0e9f};
	parameters.radius = 1.0f;

	EXPECT_THROW(auto result = spk::PoissonDisk::generate(parameters), std::length_error);
}

TEST(PoissonDiskTest, RadiusForTargetCountShrinksAsCountGrows)
{
	const spk::PoissonDisk::Size size = {100.0f, 100.0f};

	const float radiusForFew = spk::PoissonDisk::radiusForTargetCount(size, 10);
	const float radiusForMany = spk::PoissonDisk::radiusForTargetCount(size, 1000);

	EXPECT_GT(radiusForFew, 0.0f);
	EXPECT_GT(radiusForFew, radiusForMany);
}

TEST(PoissonDiskTest, RadiusForTargetCountRejectsZeroCount)
{
	EXPECT_THROW(auto result = spk::PoissonDisk::radiusForTargetCount({10.0f, 10.0f}, 0), std::invalid_argument);
}

TEST(PoissonDiskTest, RadiusForTargetCountRejectsInvalidPackingDensity)
{
	EXPECT_THROW(auto result = spk::PoissonDisk::radiusForTargetCount({10.0f, 10.0f}, 10, 0.0f), std::invalid_argument);
	EXPECT_THROW(auto result = spk::PoissonDisk::radiusForTargetCount({10.0f, 10.0f}, 10, 1.5f), std::invalid_argument);
	EXPECT_THROW(
		auto result = spk::PoissonDisk::radiusForTargetCount({10.0f, 10.0f}, 10, std::numeric_limits<float>::quiet_NaN()),
		std::invalid_argument);
}

TEST(PoissonDiskTest, RadiusForTargetCountRejectsInvalidSize)
{
	EXPECT_THROW(auto result = spk::PoissonDisk::radiusForTargetCount({0.0f, 10.0f}, 10), std::invalid_argument);
}

TEST(PoissonDiskTest, GenerateApproximateCountReturnsEmptyForZeroTarget)
{
	const std::vector<Point> points = spk::PoissonDisk::generateApproximateCount({50.0f, 50.0f}, 0);

	EXPECT_TRUE(points.empty());
}

TEST(PoissonDiskTest, GenerateApproximateCountApproachesTargetCount)
{
	const std::vector<Point> points =
		spk::PoissonDisk::generateApproximateCount({80.0f, 80.0f}, 50, 0.70f, 30, 123);

	EXPECT_FALSE(points.empty());
	EXPECT_LE(points.size(), 50u);
	// Should be in the same ballpark as the requested count.
	EXPECT_GT(points.size(), 10u);
}

TEST(PoissonDiskTest, GenerateApproximateCountForwardsAcceptPredicate)
{
	const std::vector<Point> points = spk::PoissonDisk::generateApproximateCount(
		{60.0f, 60.0f},
		40,
		0.70f,
		30,
		7,
		[](const Point &p_point) { return p_point.y < 30.0f; });

	ASSERT_FALSE(points.empty());

	for (const Point &point : points)
	{
		EXPECT_LT(point.y, 30.0f);
	}
}

TEST(PoissonDiskTest, MakePredicateFiltersAgainstGridCells)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(4, 4), 0);
	grid(1, 1) = 1;
	grid(2, 2) = 1;

	spk::PoissonDisk disk;
	const spk::PoissonDisk::PointPredicate predicate =
		disk.makePredicate(grid, [](const int &p_cell) { return p_cell == 1; });

	EXPECT_TRUE(predicate(Point(1.5f, 1.5f)));  // cell (1,1) == 1
	EXPECT_TRUE(predicate(Point(2.2f, 2.9f)));  // cell (2,2) == 1
	EXPECT_FALSE(predicate(Point(0.5f, 0.5f))); // cell (0,0) == 0
	EXPECT_FALSE(predicate(Point(-1.0f, 1.0f))); // negative coordinate
	EXPECT_FALSE(predicate(Point(1.0f, -1.0f))); // negative coordinate
	EXPECT_FALSE(predicate(Point(4.0f, 1.0f))); // x outside grid
	EXPECT_FALSE(predicate(Point(1.0f, 4.0f))); // y outside grid
}

TEST(PoissonDiskTest, MakePredicateIntegratesWithGenerate)
{
	spk::Grid2D<int> grid(spk::Vector2UInt(20, 20), 1);

	// Carve out a forbidden vertical strip.
	for (std::size_t y = 0; y < grid.height(); ++y)
	{
		for (std::size_t x = 10; x < grid.width(); ++x)
		{
			grid(x, y) = 0;
		}
	}

	spk::PoissonDisk disk;

	spk::PoissonDisk::Parameters parameters;
	parameters.size = {20.0f, 20.0f};
	parameters.radius = 1.5f;
	parameters.seed = 33;
	parameters.accept = disk.makePredicate(grid, [](const int &p_cell) { return p_cell == 1; });

	const std::vector<Point> points = spk::PoissonDisk::generate(parameters);

	ASSERT_FALSE(points.empty());

	for (const Point &point : points)
	{
		EXPECT_LT(point.x, 10.0f);
	}
}
