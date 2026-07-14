#pragma once

#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// What completing a Feat node grants. A closed variant, like the effect payloads: no reward
	// grants XP, levels, items, currency, an encounter clear, an arbitrary effect or a script.
	//
	// Rewards are replayed from a clean species baseline in board order, then node reward order,
	// and are never incrementally saved - step 17 owns that replay.

	// The amount is in the stat's own unit. Stamina is a BattleTime, so it is authored through
	// its own "amountSeconds" key and every other stat through "amount"; the unit is in the key
	// and a file cannot silently mix the two.
	struct BonusStatRewardSpec
	{
		CreatureStat stat = CreatureStat::MaxHealth;
		std::int64_t amount = 0;
		BattleTime staminaDelta;
	};

	struct UnlockAbilityRewardSpec
	{
		std::string ability;
	};

	struct RemoveAbilityRewardSpec
	{
		std::string ability;
	};

	// A passive is an infinite status (see statuses/status_definition.hpp): there is no separate
	// passive registry. A stun can never be one - a permanently stunned creature never plays.
	struct UnlockPassiveRewardSpec
	{
		std::string status;
	};

	// The form id is local to the species that later selects this board, so it is syntax-checked
	// here and resolved in step 04 through validateFeatBoardFormReferences().
	struct ChangeFormRewardSpec
	{
		std::string form;
	};

	using FeatRewardPayload = std::variant<
		BonusStatRewardSpec,
		UnlockAbilityRewardSpec,
		RemoveAbilityRewardSpec,
		UnlockPassiveRewardSpec,
		ChangeFormRewardSpec>;

	// The id is a stable semantic identity, unique across the whole board, never an array index.
	struct FeatRewardSpec
	{
		std::string id;
		FeatRewardPayload payload;
		DefinitionSource source;
	};

	// The same bound the step-02 additive stat modifier uses.
	inline constexpr std::int64_t MaximumBonusStatAmount = 1000000;

	[[nodiscard]] FeatRewardSpec parseFeatRewardSpec(JsonReader &p_reader);

	// The authored list under p_key, in order. A node may legitimately grant nothing (the root
	// usually does), so an empty list is allowed here and constrained by the node's kind.
	[[nodiscard]] std::vector<FeatRewardSpec> parseFeatRewards(const JsonReader &p_reader, const std::string &p_key);
}
