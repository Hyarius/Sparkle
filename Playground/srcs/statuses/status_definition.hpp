#pragma once

#include "abilities/effect_definition.hpp"
#include "battle/battle_types.hpp"
#include "core/json.hpp"
#include "structures/math/spk_vector2.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	enum class StatusReapplyPolicy
	{
		AddStacks,
		ReplaceStacks
	};

	enum class DurationRefreshPolicy
	{
		Replace,
		KeepLonger,
		Extend
	};

	enum class StatModifierOperation
	{
		Add,
		MultiplyPermille
	};

	// A modifier on the bearer's effective stats.
	//
	// "add" uses the stat's own unit, so a Stamina add is authored in milliseconds through the
	// explicit valueMilliseconds field and every other stat through value. "multiplyPermille"
	// always uses value, where 1000 leaves the stat unchanged.
	//
	// perStack multiplies an additive value by the current stack count with checked
	// arithmetic; on a multiplyPermille it applies the authored factor once per stack in
	// sequence, truncating after each application - it never scales the factor by the count.
	struct StatModifierSpec
	{
		std::string id;
		CreatureStat stat = CreatureStat::MaxHealth;
		StatModifierOperation operation = StatModifierOperation::Add;
		std::int64_t value = 0;
		bool perStack = false;
	};

	// The post-event moments a status may react to. There is deliberately no pre-damage
	// mutation callback: stat modifiers plus ordered post-event hooks cover v1 without a
	// hidden damage-formula rewrite.
	enum class StatusHook
	{
		Applied,
		ActivationStart,
		ActivationEnd,
		AfterAbilityCast,
		AfterDamageDealt,
		AfterDamageTaken,
		AfterHealingDone,
		AfterHealingReceived,
		AfterVoluntaryMove,
		Removed
	};

	struct StatusHookSpec
	{
		std::string id;
		StatusHook hook = StatusHook::Applied;
		std::vector<EffectApplication> effects;
	};

	// A status is also the passive model: a passive is an infinite status applied when a unit
	// is projected onto the board. There is no separate passive registry and no executable
	// passive base class.
	struct StatusDefinition
	{
		std::string id;
		// Translation keys, not sentences: the text lives in resources/data/locales.
		std::string displayNameKey;
		std::string descriptionKey;
		spk::Vector2Int icon;
		std::vector<std::string> tags;
		std::uint32_t maxStacks = 1;
		StatusReapplyPolicy reapplyPolicy = StatusReapplyPolicy::ReplaceStacks;
		DurationRefreshPolicy durationRefreshPolicy = DurationRefreshPolicy::Replace;
		std::vector<StatModifierSpec> modifiers;
		std::vector<StatusHookSpec> hooks;
	};

	inline constexpr int StatusSchemaVersion = 1;
	inline constexpr std::int64_t MaximumStatusStacks = 1000000;
	inline constexpr std::int64_t MaximumAdditiveModifier = 1000000;
	inline constexpr std::int64_t MinimumMultiplyPermille = 1;
	inline constexpr std::int64_t MaximumMultiplyPermille = 10000;

	// The reserved tag that pauses turn-bar fill. A stunned unit cannot reliably consume its
	// own turn to expire the stun, so every applyStatus reference to a stun must use a finite
	// timeline duration - checked once every status is known, in the cross-validator.
	inline constexpr std::string_view StunTag = "stun";

	[[nodiscard]] const std::map<std::string, StatusReapplyPolicy> &statusReapplyPolicyNames();
	[[nodiscard]] const std::map<std::string, DurationRefreshPolicy> &durationRefreshPolicyNames();
	[[nodiscard]] const std::map<std::string, StatModifierOperation> &statModifierOperationNames();
	[[nodiscard]] const std::map<std::string, StatusHook> &statusHookNames();

	[[nodiscard]] bool isStunStatus(const StatusDefinition &p_status) noexcept;

	// Leaves the id empty: the registry loader assigns the validated filename stem.
	[[nodiscard]] StatusDefinition parseStatusDefinition(JsonReader &p_reader);
}
