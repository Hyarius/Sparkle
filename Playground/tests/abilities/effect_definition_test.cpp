#include <gtest/gtest.h>

#include "abilities/effect_definition.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace
{
	[[nodiscard]] pg::EffectApplication parseEffect(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseEffectApplication(reader);
	}

	void expectRejected(std::string_view p_content, std::string_view p_because)
	{
		EXPECT_THROW((void)parseEffect(p_content), pg::JsonError) << "should be rejected: " << p_because;
	}

	// Throws std::bad_variant_access - and so fails the case - when the effect parsed into a
	// different alternative than the one the case names.
	template <typename TSpec>
	[[nodiscard]] const TSpec &payload(const pg::EffectApplication &p_effect)
	{
		return std::get<TSpec>(p_effect.payload);
	}
}

TEST(EffectDefinitionTest, ParsesDamage)
{
	const pg::EffectApplication effect = parseEffect(R"({
		"id": "impact",
		"type": "damage",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"kind": "physical",
		"base": 5,
		"strengthRatioPermille": 1000,
		"magicPowerRatioPermille": 0
	})");

	EXPECT_EQ(effect.id, "impact");
	EXPECT_EQ(effect.applyTo, pg::EffectApplyTo::AffectedUnits);
	EXPECT_TRUE(effect.requiresLivingSource);

	const pg::DamageEffectSpec &spec = payload<pg::DamageEffectSpec>(effect);
	EXPECT_EQ(spec.kind, pg::DamageKind::Physical);
	EXPECT_EQ(spec.base, 5);
	EXPECT_EQ(spec.strengthRatioPermille, 1000);
	EXPECT_EQ(spec.magicPowerRatioPermille, 0);
}

TEST(EffectDefinitionTest, ParsesHeal)
{
	const pg::EffectApplication effect = parseEffect(R"({
		"id": "restore",
		"type": "heal",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"base": 4,
		"strengthRatioPermille": 0,
		"magicPowerRatioPermille": 750
	})");

	const pg::HealEffectSpec &spec = payload<pg::HealEffectSpec>(effect);
	EXPECT_EQ(spec.base, 4);
	EXPECT_EQ(spec.strengthRatioPermille, 0);
	EXPECT_EQ(spec.magicPowerRatioPermille, 750);
}

TEST(EffectDefinitionTest, ParsesShieldWithItsOwnDuration)
{
	const pg::EffectApplication effect = parseEffect(R"({
		"id": "physical-ward",
		"type": "applyShield",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"kind": "physical",
		"amount": 8,
		"duration": { "type": "ownerActivations", "count": 2 }
	})");

	const pg::ApplyShieldEffectSpec &spec = payload<pg::ApplyShieldEffectSpec>(effect);
	EXPECT_EQ(spec.kind, pg::DamageKind::Physical);
	EXPECT_EQ(spec.amount, 8);
	ASSERT_TRUE(std::holds_alternative<pg::OwnerActivationsDurationSpec>(spec.duration));
	EXPECT_EQ(std::get<pg::OwnerActivationsDurationSpec>(spec.duration).count, 2U);
}

TEST(EffectDefinitionTest, ParsesApplyRemoveAndCleanseStatus)
{
	const pg::EffectApplication applied = parseEffect(R"({
		"id": "apply-poison",
		"type": "applyStatus",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"status": "poison",
		"stacks": 1,
		"duration": { "type": "timeline", "seconds": 4.0 }
	})");
	const pg::ApplyStatusEffectSpec &applySpec = payload<pg::ApplyStatusEffectSpec>(applied);
	EXPECT_EQ(applySpec.status, "poison");
	EXPECT_EQ(applySpec.stacks, 1U);
	ASSERT_TRUE(std::holds_alternative<pg::TimelineDurationSpec>(applySpec.duration));
	EXPECT_EQ(std::get<pg::TimelineDurationSpec>(applySpec.duration).duration.ticks(), 4000);

	const pg::EffectApplication removed = parseEffect(R"({
		"id": "remove-poison",
		"type": "removeStatus",
		"applyTo": "affectedUnits",
		"requiresLivingSource": false,
		"status": "poison",
		"stacks": 2
	})");
	EXPECT_FALSE(removed.requiresLivingSource);
	const pg::RemoveStatusEffectSpec &removeSpec = payload<pg::RemoveStatusEffectSpec>(removed);
	EXPECT_EQ(removeSpec.status, "poison");
	EXPECT_EQ(removeSpec.stacks, 2U);

	const pg::EffectApplication cleansed = parseEffect(R"({
		"id": "cleanse-toxins",
		"type": "cleanse",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"tags": ["poison", "toxin"]
	})");
	const pg::CleanseEffectSpec &cleanseSpec = payload<pg::CleanseEffectSpec>(cleansed);
	EXPECT_EQ(cleanseSpec.tags, (std::vector<std::string>{"poison", "toxin"}));
}

TEST(EffectDefinitionTest, KeepsACurrentResourceChangeAndANextActivationPenaltyApart)
{
	const pg::EffectApplication drain = parseEffect(R"({
		"id": "drain-current-ap",
		"type": "changeResource",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"resource": "actionPoints",
		"delta": -2
	})");
	const pg::ChangeResourceEffectSpec &drainSpec = payload<pg::ChangeResourceEffectSpec>(drain);
	EXPECT_EQ(drainSpec.resource, pg::BattleResource::ActionPoints);
	EXPECT_EQ(drainSpec.delta, -2);

	const pg::EffectApplication penalty = parseEffect(R"({
		"id": "slow-next-move",
		"type": "applyNextActivationPenalty",
		"applyTo": "affectedUnits",
		"requiresLivingSource": false,
		"resource": "movementPoints",
		"amount": 1
	})");
	const pg::ApplyNextActivationPenaltyEffectSpec &penaltySpec =
		payload<pg::ApplyNextActivationPenaltyEffectSpec>(penalty);
	EXPECT_EQ(penaltySpec.resource, pg::BattleResource::MovementPoints);
	EXPECT_EQ(penaltySpec.amount, 1);

	// The two are different alternatives, so no consumer can confuse a drained pool with a
	// reduced refill.
	EXPECT_FALSE(std::holds_alternative<pg::ChangeResourceEffectSpec>(penalty.payload));
	EXPECT_FALSE(std::holds_alternative<pg::ApplyNextActivationPenaltyEffectSpec>(drain.payload));
}

TEST(EffectDefinitionTest, ParsesTurnBarDisplacementSwapAndTeleport)
{
	const pg::EffectApplication delay = parseEffect(R"({
		"id": "delay-turn",
		"type": "adjustTurnBar",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"deltaSeconds": -0.5
	})");
	EXPECT_EQ(payload<pg::AdjustTurnBarEffectSpec>(delay).delta.ticks(), -500);

	const pg::EffectApplication push = parseEffect(R"({
		"id": "push",
		"type": "displace",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"direction": "awayFromSource",
		"distance": 2
	})");
	const pg::DisplaceEffectSpec &pushSpec = payload<pg::DisplaceEffectSpec>(push);
	EXPECT_EQ(pushSpec.direction, pg::DisplaceDirection::AwayFromSource);
	EXPECT_EQ(pushSpec.distance, 2);

	const pg::EffectApplication pull = parseEffect(R"({
		"id": "pull",
		"type": "displace",
		"applyTo": "affectedUnits",
		"requiresLivingSource": true,
		"direction": "towardSource",
		"distance": 1
	})");
	EXPECT_EQ(payload<pg::DisplaceEffectSpec>(pull).direction, pg::DisplaceDirection::TowardSource);

	const pg::EffectApplication swap = parseEffect(R"({
		"id": "transpose",
		"type": "swapWithSource",
		"applyTo": "primaryUnit",
		"requiresLivingSource": true
	})");
	EXPECT_TRUE(std::holds_alternative<pg::SwapWithSourceEffectSpec>(swap.payload));

	const pg::EffectApplication blink = parseEffect(R"({
		"id": "blink",
		"type": "teleportSource",
		"applyTo": "anchorCell",
		"requiresLivingSource": true
	})");
	EXPECT_TRUE(std::holds_alternative<pg::TeleportSourceEffectSpec>(blink.payload));
}

TEST(EffectDefinitionTest, ParsesObjectPlacementAndRemoval)
{
	const pg::EffectApplication place = parseEffect(R"({
		"id": "place-snare",
		"type": "placeObject",
		"applyTo": "anchorCell",
		"requiresLivingSource": true,
		"object": "training-snare-object",
		"duration": { "type": "ownerActivations", "count": 3 }
	})");
	const pg::PlaceObjectEffectSpec &placeSpec = payload<pg::PlaceObjectEffectSpec>(place);
	EXPECT_EQ(placeSpec.object, "training-snare-object");
	EXPECT_EQ(std::get<pg::OwnerActivationsDurationSpec>(placeSpec.duration).count, 3U);

	const pg::EffectApplication clear = parseEffect(R"({
		"id": "clear-snares",
		"type": "removeObjects",
		"applyTo": "affectedCells",
		"requiresLivingSource": false,
		"tags": ["trap"]
	})");
	EXPECT_EQ(clear.applyTo, pg::EffectApplyTo::AffectedCells);
	EXPECT_EQ(payload<pg::RemoveObjectsEffectSpec>(clear).tags, (std::vector<std::string>{"trap"}));
}

TEST(EffectDefinitionTest, ParsesEveryScopeAndEveryDurationForm)
{
	const auto scoped = [](std::string_view p_applyTo) {
		return std::string(R"({"id": "tick", "type": "damage", "applyTo": ")") + std::string(p_applyTo) +
			   R"(", "requiresLivingSource": false, "kind": "magical", "base": 1,
			   "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})";
	};
	EXPECT_EQ(parseEffect(scoped("sourceUnit")).applyTo, pg::EffectApplyTo::SourceUnit);
	EXPECT_EQ(parseEffect(scoped("primaryUnit")).applyTo, pg::EffectApplyTo::PrimaryUnit);
	EXPECT_EQ(parseEffect(scoped("affectedUnits")).applyTo, pg::EffectApplyTo::AffectedUnits);

	const auto shielded = [](std::string_view p_duration) {
		return std::string(R"({"id": "ward", "type": "applyShield", "applyTo": "sourceUnit",
			   "requiresLivingSource": true, "kind": "magical", "amount": 3, "duration": )") +
			   std::string(p_duration) + "}";
	};

	const pg::EffectApplication timeline = parseEffect(shielded(R"({"type": "timeline", "seconds": 2.5})"));
	EXPECT_EQ(
		std::get<pg::TimelineDurationSpec>(payload<pg::ApplyShieldEffectSpec>(timeline).duration).duration.ticks(),
		2500);

	const pg::EffectApplication activations =
		parseEffect(shielded(R"({"type": "ownerActivations", "count": 2})"));
	EXPECT_EQ(
		std::get<pg::OwnerActivationsDurationSpec>(payload<pg::ApplyShieldEffectSpec>(activations).duration).count,
		2U);

	const pg::EffectApplication infinite = parseEffect(shielded(R"({"type": "infinite"})"));
	EXPECT_TRUE(
		std::holds_alternative<pg::InfiniteDurationSpec>(payload<pg::ApplyShieldEffectSpec>(infinite).duration));
}

TEST(EffectDefinitionTest, RejectsAnUnknownTypeListingTheAcceptedLiterals)
{
	try
	{
		(void)parseEffect(R"({
			"id": "hex",
			"type": "runScript",
			"applyTo": "affectedUnits",
			"requiresLivingSource": true
		})");
		FAIL() << "JSON never carries executable content";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.type");
		for (const std::string_view known : {"damage", "heal", "applyStatus", "placeObject", "teleportSource"})
		{
			EXPECT_NE(error.message().find(known), std::string::npos) << "should list '" << known << "'";
		}
	}
}

TEST(EffectDefinitionTest, RejectsMissingCommonFieldsAndUnknownOnes)
{
	expectRejected(
		R"({"type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true, "kind": "physical",
		   "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a missing id");
	expectRejected(
		R"({"id": "impact", "applyTo": "affectedUnits", "requiresLivingSource": true, "kind": "physical",
		   "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a missing type");
	expectRejected(
		R"({"id": "impact", "type": "damage", "requiresLivingSource": true, "kind": "physical", "base": 1,
		   "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a missing applyTo");
	expectRejected(
		R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits", "kind": "physical", "base": 1,
		   "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a missing requiresLivingSource");
	expectRejected(
		R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0,
		   "critChance": 10})",
		"an unknown field");
	expectRejected(
		R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0,
		   "duration": {"type": "infinite"}})",
		"a field belonging to another alternative");
	expectRejected(
		R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": "yes",
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a source-liveness flag that is not a boolean");
}

TEST(EffectDefinitionTest, RejectsAnInvalidEffectIdOrReferencedId)
{
	expectRejected(
		R"({"id": "Impact!", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"an effect id outside the content-id grammar");
	expectRejected(
		R"({"id": "apply", "type": "applyStatus", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "status": "Poison", "stacks": 1, "duration": {"type": "infinite"}})",
		"a status id that is not lower case");
	expectRejected(
		R"({"id": "place", "type": "placeObject", "applyTo": "anchorCell", "requiresLivingSource": true,
		   "object": "", "duration": {"type": "infinite"}})",
		"an empty object id");
}

TEST(EffectDefinitionTest, RejectsAScopeItsPayloadCannotActuponInsteadOfReinterpretingIt)
{
	expectRejected(
		R"({"id": "impact", "type": "damage", "applyTo": "anchorCell", "requiresLivingSource": true,
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"a unit effect authored on a cell scope");
	expectRejected(
		R"({"id": "place", "type": "placeObject", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "object": "snare", "duration": {"type": "infinite"}})",
		"a cell effect authored on a unit scope");
	expectRejected(
		R"({"id": "blink", "type": "teleportSource", "applyTo": "affectedCells", "requiresLivingSource": true})",
		"teleportSource outside anchorCell: it runs once per cast, not once per cell");
	expectRejected(
		R"({"id": "transpose", "type": "swapWithSource", "applyTo": "affectedUnits",
		   "requiresLivingSource": true})",
		"swapWithSource outside primaryUnit: a swap needs exactly one partner");
}

TEST(EffectDefinitionTest, RejectsMagnitudesOutsideTheirBoundsOrThatDoNothing)
{
	const auto damage = [](std::string_view p_base, std::string_view p_strength, std::string_view p_magic) {
		return std::string(R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits",
			   "requiresLivingSource": true, "kind": "physical", "base": )") +
			   std::string(p_base) + R"(, "strengthRatioPermille": )" + std::string(p_strength) +
			   R"(, "magicPowerRatioPermille": )" + std::string(p_magic) + "}";
	};

	expectRejected(damage("-1", "1000", "0"), "a negative base");
	expectRejected(damage("1000001", "0", "0"), "a base above the maximum");
	expectRejected(damage("1", "10001", "0"), "a ratio above ten times");
	expectRejected(damage("1", "-1", "0"), "a negative ratio");
	expectRejected(damage("0", "0", "0"), "a damage effect that can never deal damage");
	EXPECT_NO_THROW((void)parseEffect(damage("0", "0", "1")));

	expectRejected(
		R"({"id": "drain", "type": "changeResource", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "resource": "actionPoints", "delta": 0})",
		"a resource change of zero");
	expectRejected(
		R"({"id": "delay", "type": "adjustTurnBar", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "deltaSeconds": 0})",
		"a turn-bar adjustment of zero");
	expectRejected(
		R"({"id": "delay", "type": "adjustTurnBar", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "deltaSeconds": 0.0005})",
		"a turn-bar adjustment finer than a millisecond");
	expectRejected(
		R"({"id": "penalty", "type": "applyNextActivationPenalty", "applyTo": "affectedUnits",
		   "requiresLivingSource": false, "resource": "movementPoints", "amount": -1})",
		"a next-activation penalty authored as a bonus");
	expectRejected(
		R"({"id": "push", "type": "displace", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "direction": "awayFromSource", "distance": 0})",
		"a displacement of zero cells");
	expectRejected(
		R"({"id": "push", "type": "displace", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "direction": "awayFromSource", "distance": 65})",
		"a displacement beyond the maximum");
	expectRejected(
		R"({"id": "apply", "type": "applyStatus", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "status": "poison", "stacks": 0, "duration": {"type": "infinite"}})",
		"applying zero stacks");
}

TEST(EffectDefinitionTest, RejectsMalformedDurationsAtTheirOwnPath)
{
	const auto shielded = [](std::string_view p_duration) {
		return std::string(R"({"id": "ward", "type": "applyShield", "applyTo": "sourceUnit",
			   "requiresLivingSource": true, "kind": "physical", "amount": 3, "duration": )") +
			   std::string(p_duration) + "}";
	};

	expectRejected(shielded(R"({"type": "turns", "count": 2})"), "an unknown duration type");
	expectRejected(shielded(R"({"type": "timeline", "seconds": 0})"), "a timeline duration of zero");
	expectRejected(shielded(R"({"type": "timeline", "seconds": -1.0})"), "a negative timeline duration");
	expectRejected(shielded(R"({"type": "timeline", "seconds": 0.0005})"), "a sub-millisecond duration");
	expectRejected(shielded(R"({"type": "timeline", "count": 2})"), "a timeline duration counting activations");
	expectRejected(shielded(R"({"type": "ownerActivations", "count": 0})"), "zero owner activations");
	expectRejected(shielded(R"({"type": "ownerActivations", "count": 1000001})"), "too many owner activations");
	expectRejected(shielded(R"({"type": "infinite", "seconds": 1.0})"), "an infinite duration with a length");

	try
	{
		(void)parseEffect(shielded(R"({"type": "timeline", "seconds": 0.0005})"));
		FAIL() << "a sub-millisecond duration must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.duration.seconds") << "the error must keep its nested path";
	}
}

TEST(EffectDefinitionTest, RejectsEmptyOrRepeatedTagFilters)
{
	expectRejected(
		R"({"id": "cleanse", "type": "cleanse", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "tags": []})",
		"a cleanse that selects nothing");
	expectRejected(
		R"({"id": "cleanse", "type": "cleanse", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "tags": ["poison", "poison"]})",
		"a repeated cleanse tag");
	expectRejected(
		R"({"id": "cleanse", "type": "cleanse", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "tags": ["Poison"]})",
		"a tag outside the content-id grammar");
	expectRejected(
		R"({"id": "clear", "type": "removeObjects", "applyTo": "affectedCells", "requiresLivingSource": false,
		   "tags": []})",
		"an object removal that selects nothing");
}

TEST(EffectDefinitionTest, KeepsTheAuthoredSourceLocationForLaterValidation)
{
	const pgtest::Document document(
		R"({"id": "impact", "type": "damage", "applyTo": "affectedUnits", "requiresLivingSource": true,
		   "kind": "physical", "base": 1, "strengthRatioPermille": 0, "magicPowerRatioPermille": 0})",
		"abilities/training-strike.json");
	pg::JsonReader reader = document.reader();

	const pg::EffectApplication effect = pg::parseEffectApplication(reader);
	EXPECT_EQ(effect.source.file, std::filesystem::path("abilities/training-strike.json"));
	EXPECT_EQ(effect.source.jsonPath, "$");
}
