#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pg
{
	// Encounter-local identity. Never a pointer, a container index, a display name or a
	// std::hash: those are unstable across a reload and cannot be compared across two runs.
	//
	// 0 is the one invalid value, so a default-constructed id can never masquerade as a real
	// one. The tag parameter keeps BattleId, BattleUnitId and BattleObjectId mutually
	// unassignable.
	template <typename TTag>
	class BattleNumericId
	{
	private:
		std::uint32_t _value = 0;

	public:
		using Value = std::uint32_t;

		constexpr BattleNumericId() noexcept = default;

		// Throws std::invalid_argument on zero: the checked public boundary. In a constant
		// expression the throw makes the construction ill-formed, so BattleId(0) cannot even
		// compile as a constant.
		explicit constexpr BattleNumericId(std::uint32_t p_value) :
			_value(p_value)
		{
			if (p_value == 0U)
			{
				throw std::invalid_argument("a battle id of zero is the invalid sentinel");
			}
		}

		[[nodiscard]] constexpr bool valid() const noexcept
		{
			return _value != 0U;
		}

		[[nodiscard]] constexpr std::uint32_t value() const noexcept
		{
			return _value;
		}

		[[nodiscard]] constexpr bool operator==(const BattleNumericId &p_other) const noexcept = default;
		[[nodiscard]] constexpr auto operator<=>(const BattleNumericId &p_other) const noexcept = default;
	};

	using BattleId = BattleNumericId<struct BattleIdTag>;
	using BattleUnitId = BattleNumericId<struct BattleUnitIdTag>;
	using BattleObjectId = BattleNumericId<struct BattleObjectIdTag>;

	// Allocation is a value, owned by whoever owns the id space - step 06 gives one to the
	// session - never a process-global counter, which would make a replay depend on how many
	// battles ran before it. Ids start at 1 and the counter is checked before it increments.
	template <typename TTag>
	class BattleNumericIdAllocator
	{
	private:
		std::uint32_t _next = 1;

	public:
		[[nodiscard]] BattleNumericId<TTag> allocate()
		{
			if (_next == 0U)
			{
				throw std::overflow_error("battle id space is exhausted");
			}

			const BattleNumericId<TTag> result(_next);
			if (_next == std::numeric_limits<std::uint32_t>::max())
			{
				_next = 0U;
			}
			else
			{
				++_next;
			}
			return result;
		}

		[[nodiscard]] std::uint32_t allocatedCount() const noexcept
		{
			return _next == 0U ? std::numeric_limits<std::uint32_t>::max() : _next - 1U;
		}
	};

	using BattleUnitIdAllocator = BattleNumericIdAllocator<BattleUnitIdTag>;
	using BattleObjectIdAllocator = BattleNumericIdAllocator<BattleObjectIdTag>;

	// The session id is not allocated from a counter: it is a pure function of the exact
	// combat seed, so an archived battle keeps its id when it is replayed. Exposed as a seam
	// so the zero fold can be tested without hunting for a seed that folds to zero.
	[[nodiscard]] BattleId battleIdFromMixedSeed(std::uint64_t p_mixed) noexcept;

	// Step 06 calls this with the combat seed step 12 derived. It never probes process
	// history for collisions: at most one BattleSession is authoritative in v1, so this
	// 32-bit value scopes ids and events inside that session; it is not a save identity.
	[[nodiscard]] BattleId deriveBattleId(std::uint64_t p_combatSeed);
}
