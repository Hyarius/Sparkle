#pragma once

#include <cstdint>

namespace pg
{
	// Produces a fresh seed for a new world. Once selected, the seed is stored in
	// WorldGenConfig and the rest of generation remains fully deterministic.
	[[nodiscard]] std::uint64_t randomWorldSeed();
}
