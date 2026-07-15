#include <gtest/gtest.h>

#include "feats/feat_reward_definition.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace
{
	[[nodiscard]] pg::FeatRewardSpec parse(std::string_view p_json)
	{
		const pgtest::Document document(p_json, "training-board.json");
		pg::JsonReader reader = document.reader();
		return pg::parseFeatRewardSpec(reader);
	}
}

TEST(FeatRewardDefinitionTest, ParsesEveryRewardAlternative)
{
	const pg::FeatRewardSpec health = parse(R"({"id": "health-up", "type": "bonusStat", "stat": "maxHealth",
		"amount": 5})");
	EXPECT_EQ(health.id, "health-up");
	const auto &healthStat = std::get<pg::BonusStatRewardSpec>(health.payload);
	EXPECT_EQ(healthStat.stat, pg::CreatureStat::MaxHealth);
	EXPECT_EQ(healthStat.amount, 5);
	EXPECT_TRUE(healthStat.staminaDelta.isZero());

	// Stamina is a BattleTime, so it is authored in seconds under its own key: the unit is in
	// the key and a file cannot silently mix the two.
	const pg::FeatRewardSpec faster = parse(R"({"id": "faster", "type": "bonusStat", "stat": "stamina",
		"amountSeconds": -0.25})");
	const auto &fasterStat = std::get<pg::BonusStatRewardSpec>(faster.payload);
	EXPECT_EQ(fasterStat.stat, pg::CreatureStat::Stamina);
	EXPECT_EQ(fasterStat.staminaDelta.ticks(), -250);
	EXPECT_EQ(fasterStat.amount, 0);

	const pg::FeatRewardSpec unlock = parse(R"({"id": "learn-guard", "type": "unlockAbility",
		"ability": "training-guard"})");
	EXPECT_EQ(std::get<pg::UnlockAbilityRewardSpec>(unlock.payload).ability, "training-guard");

	const pg::FeatRewardSpec forget = parse(R"({"id": "forget-strike", "type": "removeAbility",
		"ability": "training-strike"})");
	EXPECT_EQ(std::get<pg::RemoveAbilityRewardSpec>(forget.payload).ability, "training-strike");

	const pg::FeatRewardSpec passive = parse(R"({"id": "gain-thorns", "type": "unlockPassive",
		"status": "thorns"})");
	EXPECT_EQ(std::get<pg::UnlockPassiveRewardSpec>(passive.payload).status, "thorns");

	const pg::FeatRewardSpec form = parse(R"({"id": "become-bloom", "type": "changeForm", "form": "bloom"})");
	EXPECT_EQ(std::get<pg::ChangeFormRewardSpec>(form.payload).form, "bloom")
		<< "the form id survives parsing; step 04 resolves it against the selecting species";
	EXPECT_EQ(form.source.jsonPath, "$") << "and it keeps the path that resolution will report against";
}

TEST(FeatRewardDefinitionTest, RejectsRewardsThatGrantNothingOrTheWrongUnit)
{
	EXPECT_THROW(auto value = parse(R"({"id": "nothing", "type": "bonusStat", "stat": "armor", "amount": 0})"), pg::JsonError);
	EXPECT_THROW(
		auto value = parse(R"({"id": "nothing", "type": "bonusStat", "stat": "stamina", "amountSeconds": 0.0})"),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(R"({"id": "wrong-unit", "type": "bonusStat", "stat": "stamina", "amount": 5})"),
		pg::JsonError)
		<< "stamina is milliseconds, so it is never authored as a bare amount";
	EXPECT_THROW(
		auto value = parse(R"({"id": "wrong-unit", "type": "bonusStat", "stat": "armor", "amountSeconds": 0.5})"),
		pg::JsonError);
	EXPECT_THROW(
		auto value = parse(R"({"id": "unmeasurable", "type": "bonusStat", "stat": "stamina", "amountSeconds": 0.0005})"),
		pg::JsonError)
		<< "an authored second is exact or it is an error, never rounded";
	EXPECT_THROW(
		auto value = parse(R"({"id": "huge", "type": "bonusStat", "stat": "armor", "amount": 1000001})"),
		pg::JsonError);

	EXPECT_TRUE(std::holds_alternative<pg::BonusStatRewardSpec>(parse(R"({"id": "weaker", "type": "bonusStat", "stat": "armor", "amount": -3})").payload))
		<< "a signed amount is legal: a node may trade one stat for another";
}

TEST(FeatRewardDefinitionTest, RejectsUnknownTypesFieldsAndIds)
{
	EXPECT_THROW(auto value = parse(R"({"id": "xp", "type": "grantExperience", "amount": 100})"), pg::JsonError)
		<< "no reward grants XP, levels, items, currency or a script";
	EXPECT_THROW(
		auto value = parse(R"({"id": "learn", "type": "unlockAbility", "ability": "training-guard", "chance": 0.5})"),
		pg::JsonError);
	EXPECT_THROW(auto value = parse(R"({"id": "learn", "type": "unlockAbility"})"), pg::JsonError);
	EXPECT_THROW(auto value = parse(R"({"type": "unlockAbility", "ability": "training-guard"})"), pg::JsonError);
	EXPECT_THROW(auto value = parse(R"({"id": "Learn Guard", "type": "unlockAbility", "ability": "training-guard"})"), pg::JsonError);
	EXPECT_THROW(auto value = parse(R"({"id": "learn", "type": "unlockAbility", "ability": "Training Guard"})"), pg::JsonError);
}
