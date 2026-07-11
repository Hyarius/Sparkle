#include <gtest/gtest.h>

#include "structures/container/spk_weighted_pool.hpp"

#include <limits>
#include <stdexcept>
#include <string>

TEST(WeightedPool, PrecomputesTotalAndSelectsHalfOpenIntervals)
{
	spk::WeightedPool<std::string> pool;
	pool.add("common", 3.0);
	pool.add("rare", 1.0);

	EXPECT_DOUBLE_EQ(pool.totalWeight(), 4.0);
	EXPECT_EQ(pool.pick(0.0), "common");
	EXPECT_EQ(pool.pick(0.749), "common");
	EXPECT_EQ(pool.pick(0.75), "rare");
	EXPECT_EQ(pool.pickTarget(3.5), "rare");
}

TEST(WeightedPool, RejectsInvalidWeightsAndSelections)
{
	spk::WeightedPool<int> pool;
	EXPECT_THROW(pool.add(1, 0.0), std::invalid_argument);
	EXPECT_THROW(pool.add(1, std::numeric_limits<double>::infinity()), std::invalid_argument);
	EXPECT_THROW((void)pool.pick(0.5), std::logic_error);

	pool.add(1);
	EXPECT_THROW((void)pool.pick(-0.1), std::out_of_range);
	EXPECT_THROW((void)pool.pick(1.0), std::out_of_range);
	EXPECT_THROW((void)pool.pick(std::numeric_limits<double>::quiet_NaN()), std::out_of_range);
	EXPECT_THROW((void)pool.pickTarget(pool.totalWeight()), std::out_of_range);
}
