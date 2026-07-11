#pragma once

#include "structures/container/spk_weighted_pool.hpp"

namespace pg
{
	template <typename TValue>
	using WeightedPool = spk::WeightedPool<TValue>;
}
