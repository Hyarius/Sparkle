#pragma once

#include <cstddef>
#include <cstdint>

namespace pg
{
	// A battle draws from its own generator, seeded once from the combat seed step 12 derives
	// from the world seed and the stable encounter identity. There is no global RNG: a value
	// the session owns is what makes a battle replayable from its seed alone.
	//
	// std::random_device belongs to one place only - minting the seed of a brand new world.
	class BattleRng
	{
	private:
		std::uint64_t _state = 0;
		std::size_t _drawCount = 0;

	public:
		explicit BattleRng(std::uint64_t p_seed) noexcept;

		[[nodiscard]] std::uint64_t nextU64() noexcept;

		// Unbiased draw in [0, p_exclusiveUpperBound). Throws std::invalid_argument on a
		// bound of zero, before drawing, so a rejected call leaves the stream untouched.
		[[nodiscard]] std::uint64_t uniformBelow(std::uint64_t p_exclusiveUpperBound);

		// Counts nextU64() calls, rejected draws included. A validation or query path must
		// never make this move: an invalid command has to leave the stream exactly where it
		// found it.
		[[nodiscard]] std::size_t drawCount() const noexcept;

		[[nodiscard]] std::uint64_t state() const noexcept;
	};
}
