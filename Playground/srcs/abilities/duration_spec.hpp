#pragma once

#include "battle/battle_time.hpp"
#include "core/json.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <variant>

namespace pg
{
	// How long an authored thing - a shield, a status instance, a placed object - lasts. A
	// duration is a value, never a wall-clock deadline: timeline time advances only when the
	// scheduler advances simulated time, and owner activations are counted, not timed.
	//
	// Nothing decrements anything yet; the runtime removal order is step 09/10.

	// Simulated battle time, so a status that reads 4 s expires when the timeline has advanced
	// four seconds, whatever the frame rate.
	struct TimelineDurationSpec
	{
		BattleTime duration;
	};

	// Ordinary activations of the bearer. It decrements at the end of a completed ordinary
	// activation and never on a removal or a technical structural close.
	struct OwnerActivationsDurationSpec
	{
		std::uint32_t count = 0;
	};

	struct InfiniteDurationSpec
	{
	};

	using DurationSpec = std::variant<TimelineDurationSpec, OwnerActivationsDurationSpec, InfiniteDurationSpec>;

	enum class DurationKind
	{
		Timeline,
		OwnerActivations,
		Infinite
	};

	inline constexpr std::int64_t MinimumOwnerActivations = 1;
	inline constexpr std::int64_t MaximumOwnerActivations = 1000000;

	[[nodiscard]] const std::map<std::string, DurationKind> &durationKindNames();

	// Reads one of the three exact forms:
	//     { "type": "timeline", "seconds": 2.5 }
	//     { "type": "ownerActivations", "count": 2 }
	//     { "type": "infinite" }
	[[nodiscard]] DurationSpec parseDurationSpec(JsonReader &p_reader);
}
