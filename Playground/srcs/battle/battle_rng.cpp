#include "battle/battle_rng.hpp"

#include <limits>
#include <stdexcept>

namespace pg
{
	namespace
	{
		// SplitMix64 (Steele, Lea and Flood, 2014), the exact algorithm the golden tests pin.
		// The state advances by the odd golden-ratio gamma, and the emitted value is a
		// bijective avalanche of that new state - so the constants and the shifts below are
		// part of the save/replay contract, not an implementation detail to be tuned.
		constexpr std::uint64_t Gamma = 0x9e3779b97f4a7c15ULL;
		constexpr std::uint64_t MixA = 0xbf58476d1ce4e5b9ULL;
		constexpr std::uint64_t MixB = 0x94d049bb133111ebULL;
		constexpr unsigned int ShiftA = 30U;
		constexpr unsigned int ShiftB = 27U;
		constexpr unsigned int ShiftC = 31U;
	}

	BattleRng::BattleRng(std::uint64_t p_seed) noexcept :
		_state(p_seed)
	{
	}

	std::uint64_t BattleRng::nextU64() noexcept
	{
		_state += Gamma;
		std::uint64_t mixed = _state;
		mixed = (mixed ^ (mixed >> ShiftA)) * MixA;
		mixed = (mixed ^ (mixed >> ShiftB)) * MixB;
		++_drawCount;
		return mixed ^ (mixed >> ShiftC);
	}

	std::uint64_t BattleRng::uniformBelow(std::uint64_t p_exclusiveUpperBound)
	{
		if (p_exclusiveUpperBound == 0U)
		{
			throw std::invalid_argument("uniformBelow requires a positive exclusive upper bound");
		}

		// Rejection sampling: 2^64 is not a multiple of the bound, so the low
		// (2^64 mod bound) values would otherwise be drawn once more often than the rest.
		// Discarding exactly that prefix leaves a range the modulo divides evenly.
		const std::uint64_t rejectedPrefix =
			(std::numeric_limits<std::uint64_t>::max() - p_exclusiveUpperBound + 1U) % p_exclusiveUpperBound;

		std::uint64_t draw = nextU64();
		while (draw < rejectedPrefix)
		{
			draw = nextU64();
		}
		return draw % p_exclusiveUpperBound;
	}

	std::size_t BattleRng::drawCount() const noexcept
	{
		return _drawCount;
	}

	std::uint64_t BattleRng::state() const noexcept
	{
		return _state;
	}
}
