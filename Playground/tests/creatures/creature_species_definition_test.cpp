#include <gtest/gtest.h>

#include "core/game_rules.hpp"
#include "creatures/creature_species_definition.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] pg::GameRules rules()
	{
		pg::GameRules result;
		result.battle.minimumStamina = pg::BattleTime::fromTicks(100);
		result.battle.abilitySlotCapacity = 8;
		return result;
	}

	constexpr std::string_view BaseAttributes = R"("attributes": {
		"maxHealth": 24, "strength": 6, "magicPower": 3, "armor": 2, "resistance": 2,
		"maxActionPoints": 6, "maxMovementPoints": 4, "staminaSeconds": 4.0, "range": 0})";

	constexpr std::string_view BaseForm = R"("forms": [
		{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
		 "presentation": {"tint": [90, 190, 105, 255], "scalePermille": 1000}}])";

	// One species document with every part overridable, so a case changes exactly the field it is
	// about and nothing else can quietly drift with it.
	[[nodiscard]] std::string species(
		std::string_view p_attributes = BaseAttributes,
		std::string_view p_abilities = R"(["training-strike"])",
		std::string_view p_passives = "[]",
		std::string_view p_defaultForm = "base",
		std::string_view p_forms = BaseForm,
		std::string_view p_extra = "")
	{
		return std::string(R"({
			"version": 1,
			"displayNameKey": "creature.fixture.name",
			"descriptionKey": "creature.fixture.description",
			)") +
			   std::string(p_attributes) + R"(,
			"defaultAbilities": )" +
			   std::string(p_abilities) + R"(,
			"defaultPassives": )" +
			   std::string(p_passives) + R"(,
			"featBoard": "training-board",
			"defaultForm": ")" +
			   std::string(p_defaultForm) + R"(",
			)" +
			   std::string(p_forms) + std::string(p_extra) + "}";
	}

	[[nodiscard]] pg::CreatureSpeciesDefinition parse(const std::string &p_document)
	{
		const pgtest::Document document(p_document, "training-sprout.json");
		pg::JsonReader reader = document.reader();
		return pg::parseCreatureSpeciesDefinition(reader, rules(), pg::ConditionLimits{});
	}
}

TEST(CreatureSpeciesDefinitionTest, ParsesTheBaselineWithNoLevelAndNoRoll)
{
	const pg::CreatureSpeciesDefinition parsed = parse(species());

	EXPECT_EQ(parsed.baseAttributes.maxHealth, 24);
	EXPECT_EQ(parsed.baseAttributes.strength, 6);
	EXPECT_EQ(parsed.baseAttributes.magicPower, 3) << "the magical offense stat is magicPower, never spellPower";
	EXPECT_EQ(parsed.baseAttributes.armor, 2);
	EXPECT_EQ(parsed.baseAttributes.resistance, 2);
	EXPECT_EQ(parsed.baseAttributes.maxActionPoints, 6);
	EXPECT_EQ(parsed.baseAttributes.maxMovementPoints, 4);
	EXPECT_EQ(parsed.baseAttributes.range, 0);
	// Fixed milliseconds, never a float second: two machines have to schedule this creature the
	// same way.
	EXPECT_EQ(parsed.baseAttributes.stamina.ticks(), 4000);

	EXPECT_EQ(parsed.defaultAbilityIds, (std::vector<std::string>{"training-strike"}));
	EXPECT_TRUE(parsed.defaultPassiveStatusIds.empty());
	EXPECT_EQ(parsed.featBoardId, "training-board");
	EXPECT_EQ(parsed.defaultFormId, "base");
	ASSERT_EQ(parsed.forms.size(), 1U);
	EXPECT_EQ(parsed.forms[0].tier, 0U);
	EXPECT_EQ(parsed.forms[0].presentation.tint, (std::array<std::uint8_t, 4>{90, 190, 105, 255}));
	EXPECT_EQ(parsed.forms[0].presentation.scalePermille, 1000);
}

TEST(CreatureSpeciesDefinitionTest, RejectsAnUnknownFieldAndAWrongVersion)
{
	EXPECT_THROW(auto value = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "base", BaseForm, R"(, "level": 5)")), pg::JsonError)
		<< "there is no level in this model, so the field is simply unknown";

	const pgtest::Document document(R"({"version": 2, "displayNameKey": "creature.fixture.name"})", "sprout.json");
	pg::JsonReader reader = document.reader();
	EXPECT_THROW(auto value = pg::parseCreatureSpeciesDefinition(reader, rules(), pg::ConditionLimits{}), pg::JsonError);
}

TEST(CreatureSpeciesDefinitionTest, EnforcesEveryAttributeBoundary)
{
	const auto with = [](std::string_view p_attributes) {
		return species(p_attributes);
	};

	// Health, AP and MP are positive: a creature at zero of any of them could never take a turn.
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 0, "strength": 6, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 4,
		"staminaSeconds": 4.0, "range": 0})")),
				 pg::JsonError);
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 24, "strength": 6, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 0, "maxMovementPoints": 4,
		"staminaSeconds": 4.0, "range": 0})")),
				 pg::JsonError);
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 24, "strength": 6, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 0,
		"staminaSeconds": 4.0, "range": 0})")),
				 pg::JsonError);

	// The rest are non-negative, and all of them are capped.
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 24, "strength": -1, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 4,
		"staminaSeconds": 4.0, "range": 0})")),
				 pg::JsonError);
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 1000001, "strength": 6, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 4,
		"staminaSeconds": 4.0, "range": 0})")),
				 pg::JsonError);

	// A missing attribute is not defaulted into existence.
	EXPECT_THROW(auto value = parse(with(R"("attributes": {"maxHealth": 24, "strength": 6, "magicPower": 3,
		"armor": 2, "resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 4,
		"staminaSeconds": 4.0})")),
				 pg::JsonError);
}

TEST(CreatureSpeciesDefinitionTest, ParsesStaminaExactlyAndRefusesToBeFasterThanTheRules)
{
	const auto withStamina = [](std::string_view p_seconds) {
		return species(
			std::string(R"("attributes": {"maxHealth": 24, "strength": 6, "magicPower": 3, "armor": 2,
			"resistance": 2, "maxActionPoints": 6, "maxMovementPoints": 4, "staminaSeconds": )") +
			std::string(p_seconds) + R"(, "range": 0})");
	};

	EXPECT_EQ(parse(withStamina("0.25")).baseAttributes.stamina.ticks(), 250);
	EXPECT_EQ(parse(withStamina("0.1")).baseAttributes.stamina.ticks(), 100) << "exactly the rules' minimum";

	// A fraction of a millisecond is rejected rather than rounded away.
	EXPECT_THROW(auto value = parse(withStamina("0.0005")), pg::JsonError);
	// Faster than the rules allow, and zero or negative, are all refused.
	EXPECT_THROW(auto value = parse(withStamina("0.05")), pg::JsonError);
	EXPECT_THROW(auto value = parse(withStamina("0.0")), pg::JsonError);
	EXPECT_THROW(auto value = parse(withStamina("-1.0")), pg::JsonError);
}

TEST(CreatureSpeciesDefinitionTest, EnforcesTheDefaultAbilityAndPassiveLists)
{
	// A creature that knows nothing cannot fight.
	EXPECT_THROW(auto value = parse(species(BaseAttributes, "[]")), pg::JsonError);
	// Two slots holding the same ability is an authoring mistake, not a stack.
	EXPECT_THROW(
		auto value = parse(species(BaseAttributes, R"(["training-strike", "training-strike"])")),
		pg::JsonError);
	EXPECT_THROW(auto value = parse(species(BaseAttributes, R"(["Training_Strike"])")), pg::JsonError)
		<< "ids are the step-01 grammar, never case-folded into shape";
	// Over the loadout array, before any board grants a thing.
	EXPECT_THROW(
		auto value = parse(species(
			BaseAttributes,
			R"(["a-one", "a-two", "a-three", "a-four", "a-five", "a-six", "a-seven", "a-eight", "a-nine"])")),
		pg::JsonError);

	EXPECT_THROW(
		auto value = parse(species(BaseAttributes, R"(["training-strike"])", R"(["p-one", "p-one"])")),
		pg::JsonError);
	EXPECT_NO_THROW(auto value = parse(species(BaseAttributes, R"(["training-strike"])", R"(["training-guarded"])")));
}

TEST(CreatureSpeciesDefinitionTest, EnforcesTheFormAndTierRules)
{
	constexpr std::string_view TwoForms = R"("forms": [
		{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
		 "presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}},
		{"id": "grown", "displayNameKey": "creature.fixture.form.grown.name", "tier": 1,
		 "presentation": {"tint": [4, 5, 6, 255], "scalePermille": 1400}}])";

	const pg::CreatureSpeciesDefinition parsed = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "base", TwoForms));
	ASSERT_EQ(parsed.forms.size(), 2U);
	EXPECT_EQ(parsed.forms[0].id, "base") << "authored order is preserved";
	EXPECT_EQ(parsed.forms[1].id, "grown");
	EXPECT_NE(parsed.tryForm("grown"), nullptr);
	EXPECT_EQ(parsed.tryForm("ghost"), nullptr);

	// The default form is the tier-0 one, and there is exactly one of those.
	EXPECT_THROW(
		auto value = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "grown", TwoForms)),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "ghost", TwoForms)),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(species(
			BaseAttributes,
			R"(["training-strike"])",
			"[]",
			"base",
			R"("forms": [
				{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
				 "presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}},
				{"id": "twin", "displayNameKey": "creature.fixture.form.twin.name", "tier": 0,
				 "presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}}])")),
		pg::JsonError)
		<< "two tier-0 forms leave 'which one does a fresh creature wear' unanswered";

	// A species has at least one form, and its ids are unique.
	EXPECT_THROW(auto value = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "base", R"("forms": [])")), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(species(
			BaseAttributes,
			R"(["training-strike"])",
			"[]",
			"base",
			R"("forms": [
				{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 0,
				 "presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}},
				{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 1,
				 "presentation": {"tint": [1, 2, 3, 255], "scalePermille": 1000}}])")),
		pg::JsonError);
}

TEST(CreatureSpeciesDefinitionTest, EnforcesThePlaceholderPresentationBounds)
{
	const auto withPresentation = [](std::string_view p_presentation) {
		return species(
			BaseAttributes,
			R"(["training-strike"])",
			"[]",
			"base",
			std::string(R"("forms": [{"id": "base", "displayNameKey": "creature.fixture.form.base.name",
				"tier": 0, "presentation": )") +
				std::string(p_presentation) + "}]");
	};

	EXPECT_NO_THROW(auto value = parse(withPresentation(R"({"tint": [0, 0, 0, 0], "scalePermille": 250})")));
	EXPECT_NO_THROW(auto value = parse(withPresentation(R"({"tint": [255, 255, 255, 255], "scalePermille": 3000})")));

	EXPECT_THROW(auto value = parse(withPresentation(R"({"tint": [256, 0, 0, 0], "scalePermille": 1000})")), pg::JsonError);
	EXPECT_THROW(auto value = parse(withPresentation(R"({"tint": [-1, 0, 0, 0], "scalePermille": 1000})")), pg::JsonError);
	EXPECT_THROW(auto value = parse(withPresentation(R"({"tint": [0, 0, 0], "scalePermille": 1000})")), pg::JsonError)
		<< "four channels, always";
	EXPECT_THROW(auto value = parse(withPresentation(R"({"tint": [0, 0, 0, 0], "scalePermille": 249})")), pg::JsonError);
	EXPECT_THROW(auto value = parse(withPresentation(R"({"tint": [0, 0, 0, 0], "scalePermille": 3001})")), pg::JsonError);
	// The block is headless data, so it holds no renderer key at all.
	EXPECT_THROW(
		auto value = parse(withPresentation(R"({"tint": [0, 0, 0, 0], "scalePermille": 1000, "mesh": "sprout.obj"})")),
		pg::JsonError);

	// Tier is bounded too.
	EXPECT_THROW(
		auto value = parse(species(
			BaseAttributes,
			R"(["training-strike"])",
			"[]",
			"base",
			R"("forms": [{"id": "base", "displayNameKey": "creature.fixture.form.base.name", "tier": 101,
				"presentation": {"tint": [0, 0, 0, 0], "scalePermille": 1000}}])")),
		pg::JsonError);
}

TEST(CreatureSpeciesDefinitionTest, TreatsAnAbsentTamingProfileAsUntameable)
{
	EXPECT_FALSE(parse(species()).tamingProfile.has_value());

	// An explicit null is not "absent": in version 1 it is a value of the wrong type.
	EXPECT_THROW(
		auto value = parse(species(BaseAttributes, R"(["training-strike"])", "[]", "base", BaseForm, R"(, "tamingProfile": null)")),
		pg::JsonError);

	constexpr std::string_view Profile = R"(, "tamingProfile": {"conditions": [
		{"id": "approach-carefully", "type": "position", "descriptionKey": "taming.fixture.description",
		 "window": "turn", "windowActor": "opponentTeam", "requiredWindowCount": 1,
		 "windowMode": "cumulative", "sample": "afterMove", "actor": "opponentTeam",
		 "relativeTo": "subject", "comparison": "atMost", "distance": 1}]})";

	const pg::CreatureSpeciesDefinition tameable =
		parse(species(BaseAttributes, R"(["training-strike"])", "[]", "base", BaseForm, Profile));
	ASSERT_TRUE(tameable.tamingProfile.has_value());
	EXPECT_EQ(tameable.tamingProfile->conditions.size(), 1U);

	// A present profile means tameable, so an empty condition list is an authoring error.
	EXPECT_THROW(
		auto value = parse(species(
			BaseAttributes,
			R"(["training-strike"])",
			"[]",
			"base",
			BaseForm,
			R"(, "tamingProfile": {"conditions": []})")),
		pg::JsonError);
}
