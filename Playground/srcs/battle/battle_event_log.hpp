#pragma once

#include "battle/battle_command_result.hpp" // EventRange
#include "battle/battle_event.hpp"
#include "battle/battle_snapshot.hpp"

#include <optional>
#include <span>
#include <vector>

namespace pg
{
	class BattleContext;

	// Explicit authoritative metadata: a batch's kind is never inferred from a nullable action id or
	// from the categories of the events it contains.
	enum class BattleBatchKind
	{
		Construction, // one at session creation: BattleStarted plus system enemy placement
		Command,      // one per accepted public command; carries a non-null action id
		Timeline,     // step 07 timeline transitions; no action id
		TamingSystem, // step 16 post-condition removals; no action id
		TechnicalAbort // the reserved no-action abort tail
	};

	[[nodiscard]] std::string_view toString(BattleBatchKind p_kind) noexcept;

	// One committed batch as an immutable archive record: its id/kind/optional action, the event
	// range it owns and the before/after simulation snapshots it froze. Result transcripts and
	// replay inspect these; they never reconstruct batch boundaries from the event stream.
	struct CommittedBattleBatch
	{
		BattleBatchId id;
		BattleBatchKind kind = BattleBatchKind::Command;
		std::optional<BattleActionId> action;
		EventRange events;
		BattleSnapshot before;
		BattleSnapshot after;
	};

	// The append-only authoritative event stream. Only BattleContext (the private writer owned by the
	// session) may append; everyone else copies out of it. Nothing here calls EventCenter, a
	// ContractProvider, a widget or a Feat/taming service: publication is the session's later,
	// separate concern.
	class BattleEventLog
	{
		friend class BattleContext;

	private:
		std::vector<BattleEvent> _events;

		// The sole append surface, reachable only through the friend writer.
		void append(BattleEvent p_event);

	public:
		[[nodiscard]] std::span<const BattleEvent> events() const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
		// Copies the half-open [first, onePastLast) range out by value. Throws std::out_of_range on a
		// range outside the committed stream.
		[[nodiscard]] std::vector<BattleEvent> copy(EventRange p_range) const;
	};
}
