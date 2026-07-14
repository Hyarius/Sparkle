#pragma once

#include "abilities/duration_spec.hpp"
#include "battle/battle_time.hpp"
#include "battle/battle_types.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace pg
{
	// Where an effect lands. The resolver captures its targets and cells once per cast, in
	// canonical order; an effect never re-queries an expanding area after a displacement. The
	// concrete meaning of each scope depends on the owner (ability, status hook, object
	// trigger) and is contracted in the plan - steps 08-10 implement it. Nothing executes yet.
	enum class EffectApplyTo
	{
		SourceUnit,
		PrimaryUnit,
		AffectedUnits,
		AnchorCell,
		AffectedCells
	};

	enum class DisplaceDirection
	{
		AwayFromSource,
		TowardSource
	};

	// Physical damage is mitigated by Armor and magical damage by Resistance, but both ratios
	// stay explicit so an unusual hybrid stays authored data rather than special-cased C++.
	// Step 09 owns the checked formula.
	struct DamageEffectSpec
	{
		DamageKind kind = DamageKind::Physical;
		std::int64_t base = 0;
		std::int32_t strengthRatioPermille = 0;
		std::int32_t magicPowerRatioPermille = 0;
	};

	// Healing does not revive in v1 and clamps at the effective maximum health.
	struct HealEffectSpec
	{
		std::int64_t base = 0;
		std::int32_t strengthRatioPermille = 0;
		std::int32_t magicPowerRatioPermille = 0;
	};

	// A shield of a matching damage kind absorbs before HP.
	struct ApplyShieldEffectSpec
	{
		DamageKind kind = DamageKind::Physical;
		std::int64_t amount = 0;
		DurationSpec duration;
	};

	struct ApplyStatusEffectSpec
	{
		std::string status;
		std::uint32_t stacks = 1;
		DurationSpec duration;
	};

	struct RemoveStatusEffectSpec
	{
		std::string status;
		std::uint32_t stacks = 1;
	};

	// Removes every status instance carrying any of these tags.
	struct CleanseEffectSpec
	{
		std::vector<std::string> tags;
	};

	// Changes the current AP/MP pool now. It is deliberately a different effect from a next
	// activation penalty, and the two must stay distinguishable through events and tests.
	struct ChangeResourceEffectSpec
	{
		BattleResource resource = BattleResource::ActionPoints;
		std::int64_t delta = 0;
	};

	// Subtracted from the refill at exactly the next activation start, then consumed. It can
	// never be authored as a bonus: bonuses and maximum changes are statuses and stat
	// modifiers.
	struct ApplyNextActivationPenaltyEffectSpec
	{
		BattleResource resource = BattleResource::ActionPoints;
		std::int64_t amount = 0;
	};

	// Positive adds turn-bar fill and brings readiness closer, negative delays. It never
	// changes the authored stamina.
	struct AdjustTurnBarEffectSpec
	{
		BattleTime delta;
	};

	struct DisplaceEffectSpec
	{
		DisplaceDirection direction = DisplaceDirection::AwayFromSource;
		int distance = 1;
	};

	// Source and primary unit trade cells atomically. No extra fields.
	struct SwapWithSourceEffectSpec
	{
	};

	// The ability-selected anchor is the destination; no absolute cell is ever authored. It
	// executes once per cast, not once per captured cell.
	struct TeleportSourceEffectSpec
	{
	};

	struct PlaceObjectEffectSpec
	{
		std::string object;
		DurationSpec duration;
	};

	// Removes every placed object carrying any of these tags.
	struct RemoveObjectsEffectSpec
	{
		std::vector<std::string> tags;
	};

	// A closed variant: "type" maps to a compile-time-known alternative, an unknown type is a
	// hard error, and there is no general expression or script payload. JSON never carries
	// executable content.
	using EffectPayload = std::variant<
		DamageEffectSpec,
		HealEffectSpec,
		ApplyShieldEffectSpec,
		ApplyStatusEffectSpec,
		RemoveStatusEffectSpec,
		CleanseEffectSpec,
		ChangeResourceEffectSpec,
		ApplyNextActivationPenaltyEffectSpec,
		AdjustTurnBarEffectSpec,
		DisplaceEffectSpec,
		SwapWithSourceEffectSpec,
		TeleportSourceEffectSpec,
		PlaceObjectEffectSpec,
		RemoveObjectsEffectSpec>;

	// Where the effect was authored, so the cross-validation phase can point at the line the
	// author has to fix rather than at the registry.
	using EffectSource = DefinitionSource;

	// One authored effect. The id is a stable semantic identity for events and debugging, not
	// an array index, and it is unique across its whole owning definition.
	struct EffectApplication
	{
		std::string id;
		EffectApplyTo applyTo = EffectApplyTo::AffectedUnits;
		// An already captured cast continues after its source is defeated unless the effect
		// says it needs the source alive. A copied source id does not prove the source still
		// exists - this flag decides whether execution may continue.
		bool requiresLivingSource = true;
		EffectPayload payload;
		EffectSource source;
	};

	inline constexpr std::int64_t MaximumEffectMagnitude = 1000000;
	inline constexpr std::int64_t MaximumEffectRatioPermille = 10000;
	inline constexpr std::int64_t MaximumDisplaceDistance = 64;

	[[nodiscard]] const std::map<std::string, EffectApplyTo> &effectApplyToNames();
	[[nodiscard]] const std::map<std::string, DisplaceDirection> &displaceDirectionNames();

	[[nodiscard]] EffectApplication parseEffectApplication(JsonReader &p_reader);

	// The authored list under p_key, in order. Every owner of an effect list - an ability, a
	// status hook, an object trigger - requires it to do something, so an empty list is an
	// error rather than a silent no-op.
	[[nodiscard]] std::vector<EffectApplication> parseEffects(const JsonReader &p_reader, const std::string &p_key);

	// Effect ids are unique across the whole owning definition, including effects sitting in
	// different status hooks or object triggers. p_seenIds accumulates across those lists.
	void requireUniqueEffectIds(const std::vector<EffectApplication> &p_effects, std::set<std::string> &p_seenIds);
}
