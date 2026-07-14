#include <gtest/gtest.h>

#include "core/game_rules.hpp"
#include "core/json.hpp"
#include "core/paths.hpp"

#include <array>
#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] std::filesystem::path rulesFile()
	{
		return pg::resourceRoot() / "data" / "config" / "game-rules.json";
	}

	// Every fixture starts from the shipped file, so a case only ever states the one thing it
	// breaks - and a schema addition cannot leave a fixture silently testing an old shape.
	[[nodiscard]] spk::JSON::Value baseRules()
	{
		return pg::JsonLoader::parseFile(rulesFile());
	}

	[[nodiscard]] pg::GameRules parse(const spk::JSON::Value &p_value)
	{
		pg::JsonReader reader(p_value, "game-rules.json");
		return pg::parseGameRules(reader);
	}

	void expectRejected(const spk::JSON::Value &p_value, std::string_view p_because)
	{
		EXPECT_THROW((void)parse(p_value), pg::JsonError) << "should be rejected: " << p_because;
	}

	[[nodiscard]] spk::JSON::Value cell(std::initializer_list<int> p_coordinates)
	{
		spk::JSON::Value result = spk::JSON::Value::array();
		for (const int coordinate : p_coordinates)
		{
			result.pushBack(spk::JSON::Value(coordinate));
		}
		return result;
	}
}

TEST(GameRulesTest, LoadsTheShippedVersionTwoFile)
{
	const pg::GameRules rules = parse(baseRules());

	EXPECT_DOUBLE_EQ(rules.maxVerticalTraversalGap, 0.5);

	EXPECT_EQ(rules.battle.teamCapacity, 6U);
	EXPECT_EQ(rules.battle.abilitySlotCapacity, 8U);
	EXPECT_EQ(rules.battle.defaultBoardSize[0], 11);
	EXPECT_EQ(rules.battle.defaultBoardSize[1], 11);
	EXPECT_EQ(rules.battle.deploymentDepth, 2);
	EXPECT_EQ(rules.battle.minimumStamina.ticks(), 100);
	EXPECT_EQ(rules.battle.mitigationScale, 100);
	EXPECT_EQ(rules.battle.maxCommandsPerActivation, 64U);
	EXPECT_EQ(rules.battle.maxAiCommandsPerActivation, 32U);
	EXPECT_EQ(rules.battle.maxEffectChainDepth, 32U);
	EXPECT_EQ(rules.battle.maxConditionDepth, 8U);

	EXPECT_EQ(rules.overlayMasks.size(), 8U);
	EXPECT_EQ(rules.overlayMasks.at("deployment"), (std::array<int, 2>{0, 0}));
	EXPECT_EQ(rules.overlayMasks.at("movement"), (std::array<int, 2>{1, 0}));
	EXPECT_EQ(rules.overlayMasks.at("path"), (std::array<int, 2>{2, 0}));
	EXPECT_EQ(rules.overlayMasks.at("abilityRange"), (std::array<int, 2>{3, 0}));
	EXPECT_EQ(rules.overlayMasks.at("areaOfEffect"), (std::array<int, 2>{0, 1}));
	EXPECT_EQ(rules.overlayMasks.at("losBlocked"), (std::array<int, 2>{1, 1}));
	// The two cells the exploration prototype already draws keep their authored values.
	EXPECT_EQ(rules.overlayMasks.at("hovered"), (std::array<int, 2>{2, 1}));
	EXPECT_EQ(rules.overlayMasks.at("invalid"), (std::array<int, 2>{3, 1}));
}

TEST(GameRulesTest, RejectsAnyVersionButTwo)
{
	for (const int version : {0, 1, 3, 200})
	{
		spk::JSON::Value rules = baseRules();
		rules["version"] = version;
		expectRejected(rules, "version " + std::to_string(version));
	}
	// Version 1 is not parsed for compatibility: the resource file migrates in the same
	// commit, so an old file must fail loudly rather than half-load.
}

TEST(GameRulesTest, RejectsMissingFieldsAtEveryLevel)
{
	for (const std::string_view key : {"version", "maxVerticalTraversalGap", "battle", "overlayMasks"})
	{
		spk::JSON::Value rules = baseRules();
		rules.asObject().erase(std::string(key));
		expectRejected(rules, std::string("missing root field '") + std::string(key) + "'");
	}

	for (const std::string_view key :
		 {"teamCapacity",
		  "abilitySlotCapacity",
		  "defaultBoardSize",
		  "deploymentDepth",
		  "minimumStaminaSeconds",
		  "mitigationScale",
		  "maxCommandsPerActivation",
		  "maxAiCommandsPerActivation",
		  "maxEffectChainDepth",
		  "maxConditionDepth"})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"].asObject().erase(std::string(key));
		expectRejected(rules, std::string("missing battle field '") + std::string(key) + "'");
	}

	for (const std::string_view key :
		 {"deployment", "movement", "path", "abilityRange", "areaOfEffect", "losBlocked", "hovered", "invalid"})
	{
		spk::JSON::Value rules = baseRules();
		rules["overlayMasks"].asObject().erase(std::string(key));
		expectRejected(rules, std::string("missing mask '") + std::string(key) + "'");
	}
}

TEST(GameRulesTest, RejectsUnknownFieldsAtEveryLevel)
{
	spk::JSON::Value rootExtra = baseRules();
	rootExtra["mitigationScaling"] = 10;
	expectRejected(rootExtra, "an unknown root field");

	spk::JSON::Value battleExtra = baseRules();
	battleExtra["battle"]["initiativeSpeed"] = 4;
	expectRejected(battleExtra, "an unknown battle field");

	spk::JSON::Value maskExtra = baseRules();
	maskExtra["overlayMasks"]["taunt"] = cell({0, 2});
	expectRejected(maskExtra, "an unknown overlay mask");
}

TEST(GameRulesTest, RejectsAGapThatIsNotAFiniteNonNegativeNumber)
{
	spk::JSON::Value negative = baseRules();
	negative["maxVerticalTraversalGap"] = -0.5;
	expectRejected(negative, "a negative traversal gap");

	spk::JSON::Value text = baseRules();
	text["maxVerticalTraversalGap"] = "half";
	expectRejected(text, "a traversal gap that is not a number");
}

TEST(GameRulesTest, RejectsATeamCapacityOtherThanSix)
{
	for (const int capacity : {0, 5, 7, 12})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["teamCapacity"] = capacity;
		expectRejected(rules, "team capacity " + std::to_string(capacity));
	}
}

TEST(GameRulesTest, RejectsAnAbilitySlotCapacityOtherThanEight)
{
	for (const int capacity : {0, 4, 7, 9})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["abilitySlotCapacity"] = capacity;
		expectRejected(rules, "ability slot capacity " + std::to_string(capacity));
	}
}

TEST(GameRulesTest, RejectsBoardsThatAreNotOddAndInRange)
{
	const std::array<std::pair<int, int>, 6> invalidBoards{
		std::pair{10, 11}, // even width
		std::pair{11, 10}, // even depth
		std::pair{3, 11},  // narrower than the minimum
		std::pair{11, 3},  // shallower than the minimum
		std::pair{33, 11}, // wider than the maximum
		std::pair{11, 33}  // deeper than the maximum
	};

	for (const auto &[width, depth] : invalidBoards)
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["defaultBoardSize"] = cell({width, depth});
		expectRejected(rules, "board " + std::to_string(width) + "x" + std::to_string(depth));
	}

	spk::JSON::Value shortArray = baseRules();
	shortArray["battle"]["defaultBoardSize"] = cell({11});
	expectRejected(shortArray, "a board size with one dimension");

	spk::JSON::Value longArray = baseRules();
	longArray["battle"]["defaultBoardSize"] = cell({11, 11, 11});
	expectRejected(longArray, "a board size with three dimensions");
}

TEST(GameRulesTest, RejectsDeploymentDepthsThatLeaveNoNeutralRow)
{
	for (const int depth : {0, -1})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["deploymentDepth"] = depth;
		expectRejected(rules, "deployment depth " + std::to_string(depth));
	}

	// Board depth 11: two strips of 5 still leave the centre row, two of 6 collide.
	spk::JSON::Value fits = baseRules();
	fits["battle"]["deploymentDepth"] = 5;
	EXPECT_NO_THROW((void)parse(fits));

	spk::JSON::Value collides = baseRules();
	collides["battle"]["deploymentDepth"] = 6;
	expectRejected(collides, "two strips of 6 on a board 11 deep");
}

TEST(GameRulesTest, RejectsAMinimumStaminaThatIsNotAPositiveWholeMillisecond)
{
	spk::JSON::Value subMillisecond = baseRules();
	subMillisecond["battle"]["minimumStaminaSeconds"] = 0.0005;
	expectRejected(subMillisecond, "a sub-millisecond stamina floor");

	spk::JSON::Value zero = baseRules();
	zero["battle"]["minimumStaminaSeconds"] = 0;
	expectRejected(zero, "a stamina floor of zero");

	spk::JSON::Value negative = baseRules();
	negative["battle"]["minimumStaminaSeconds"] = -0.1;
	expectRejected(negative, "a negative stamina floor");

	// Exactly one tick is the smallest accepted floor.
	spk::JSON::Value oneTick = baseRules();
	oneTick["battle"]["minimumStaminaSeconds"] = 0.001;
	EXPECT_EQ(parse(oneTick).battle.minimumStamina.ticks(), 1);
}

TEST(GameRulesTest, RejectsAMitigationScaleOutsideItsBounds)
{
	for (const int scale : {0, -100})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["mitigationScale"] = scale;
		expectRejected(rules, "mitigation scale " + std::to_string(scale));
	}

	spk::JSON::Value tooLarge = baseRules();
	tooLarge["battle"]["mitigationScale"] = 1000001;
	expectRejected(tooLarge, "a mitigation scale above the maximum");

	spk::JSON::Value fractional = baseRules();
	fractional["battle"]["mitigationScale"] = 10.5;
	expectRejected(fractional, "a fractional mitigation scale");

	spk::JSON::Value atMaximum = baseRules();
	atMaximum["battle"]["mitigationScale"] = 1000000;
	EXPECT_EQ(parse(atMaximum).battle.mitigationScale, 1000000);
}

TEST(GameRulesTest, RejectsCommandAndDepthLimitsOutsideTheirBounds)
{
	spk::JSON::Value zeroCommands = baseRules();
	zeroCommands["battle"]["maxCommandsPerActivation"] = 0;
	expectRejected(zeroCommands, "a command budget of zero");

	spk::JSON::Value zeroAiCommands = baseRules();
	zeroAiCommands["battle"]["maxAiCommandsPerActivation"] = 0;
	expectRejected(zeroAiCommands, "an AI command budget of zero");

	// The AI may never be allowed more commands than a player activation permits.
	spk::JSON::Value greedyAi = baseRules();
	greedyAi["battle"]["maxAiCommandsPerActivation"] = 65;
	expectRejected(greedyAi, "an AI budget above the command budget");

	spk::JSON::Value equalAi = baseRules();
	equalAi["battle"]["maxAiCommandsPerActivation"] = 64;
	EXPECT_EQ(parse(equalAi).battle.maxAiCommandsPerActivation, 64U);

	for (const int depth : {0, 257})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["maxEffectChainDepth"] = depth;
		expectRejected(rules, "effect chain depth " + std::to_string(depth));
	}

	for (const int depth : {0, 33})
	{
		spk::JSON::Value rules = baseRules();
		rules["battle"]["maxConditionDepth"] = depth;
		expectRejected(rules, "condition depth " + std::to_string(depth));
	}
}

TEST(GameRulesTest, RejectsMalformedMaskCells)
{
	spk::JSON::Value negative = baseRules();
	negative["overlayMasks"]["hovered"] = cell({-1, 1});
	expectRejected(negative, "a negative mask coordinate");

	spk::JSON::Value single = baseRules();
	single["overlayMasks"]["movement"] = cell({1});
	expectRejected(single, "a mask cell with one coordinate");

	spk::JSON::Value triple = baseRules();
	triple["overlayMasks"]["movement"] = cell({1, 0, 3});
	expectRejected(triple, "a mask cell with three coordinates");

	spk::JSON::Value text = baseRules();
	text["overlayMasks"]["path"] = "2,0";
	expectRejected(text, "a mask cell that is not an array");
}

TEST(GameRulesTest, ErrorsNameTheFileAndTheExactJsonPath)
{
	spk::JSON::Value rules = baseRules();
	rules["battle"]["teamCapacity"] = 5;

	try
	{
		(void)parse(rules);
		FAIL() << "a team capacity of 5 must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file(), std::filesystem::path("game-rules.json"));
		EXPECT_EQ(error.path(), "$.battle.teamCapacity");
		EXPECT_NE(error.message().find('6'), std::string::npos);
	}
}
