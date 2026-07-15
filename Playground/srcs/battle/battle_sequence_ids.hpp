#pragma once

#include <compare>
#include <cstdint>

namespace pg
{
	// The runtime sequence identities of one battle, all scoped to a single BattleId. They are
	// never persistent save identifiers and are never raw array positions: a rejected command must
	// leave every next counter untouched, which a plain vector index can never promise.
	//
	// 0 is the unallocated sentinel on all four; accepted commands, committed batches and committed
	// events each start at 1. TurnIndex is declared here because every event header carries an
	// optional turn, but step 07 assigns its first non-zero value.

	struct BattleActionId
	{
		std::uint64_t value = 0; // 0 invalid; accepted commands start at 1
		[[nodiscard]] auto operator<=>(const BattleActionId &) const = default;
	};

	struct BattleBatchId
	{
		std::uint64_t value = 0; // committed batches start at 1
		[[nodiscard]] auto operator<=>(const BattleBatchId &) const = default;
	};

	struct BattleEventSequence
	{
		std::uint64_t value = 0; // committed events start at 1
		[[nodiscard]] auto operator<=>(const BattleEventSequence &) const = default;
	};

	struct TurnIndex
	{
		std::uint64_t value = 0; // activations start at 1 in step 07
		[[nodiscard]] auto operator<=>(const TurnIndex &) const = default;
	};
}
