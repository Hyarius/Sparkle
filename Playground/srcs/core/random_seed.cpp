#include "core/random_seed.hpp"

#include <random>

namespace pg
{
	std::uint64_t randomWorldSeed()
	{
		std::random_device device;
		return (static_cast<std::uint64_t>(device()) << 32) | static_cast<std::uint64_t>(device());
	}
}
