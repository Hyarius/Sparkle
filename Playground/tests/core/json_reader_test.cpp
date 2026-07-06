#include "core/game_rules.hpp"
#include "core/json.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>

namespace
{
	enum class TestMode
	{
		Fast,
		Safe
	};

	template <typename TFunction>
	void expectJsonErrorAt(TFunction p_function, const std::string &p_path)
	{
		try
		{
			p_function();
			FAIL() << "Expected pg::JsonError";
		} catch (const pg::JsonError &error)
		{
			EXPECT_NE(std::string(error.what()).find("sample.json:" + p_path), std::string::npos) << error.what();
		}
	}

	nlohmann::json validGameRulesJson()
	{
		return nlohmann::json::parse(R"json(
			{
				"version": 1,
				"maxVerticalTraversalGap": 0.5,
				"overlayMasks": {
					"hovered": [2, 1],
					"invalid": [3, 1]
				}
			}
		)json");
	}
}

TEST(JsonReader, ReadsRequiredOptionalEnumAndChildren)
{
	const nlohmann::json json = {
		{"name", "sample"},
		{"mode", "fast"},
		{"child", {{"value", 7}}},
		{"children", {{{"value", 1}}, {{"value", 2}}}}};
	pg::JsonReader reader(json, "sample.json");

	EXPECT_EQ(reader.require<std::string>("name"), "sample");
	EXPECT_EQ(reader.optional<int>("count", 4), 4);
	EXPECT_EQ(
		reader.requireEnum<TestMode>("mode", std::map<std::string, TestMode>{{"fast", TestMode::Fast}, {"safe", TestMode::Safe}}),
		TestMode::Fast);
	EXPECT_EQ(reader.child("child").require<int>("value"), 7);
	const std::vector<pg::JsonReader> children = reader.childArray("children");
	ASSERT_EQ(children.size(), 2);
	EXPECT_EQ(children[1].require<int>("value"), 2);
}

TEST(JsonReader, ReportsMissingFieldPath)
{
	const nlohmann::json json = nlohmann::json::object();
	const pg::JsonReader reader(json, "sample.json");
	expectJsonErrorAt([&reader]() {
		(void)reader.require<int>("missing");
	},
					  "$.missing");
}

TEST(JsonReader, ReportsUnknownFieldPath)
{
	const nlohmann::json json = {{"known", 1}, {"extra", 2}};
	const pg::JsonReader reader(json, "sample.json");
	expectJsonErrorAt([&reader]() {
		reader.forbidUnknown({"known"});
	},
					  "$.extra");
}

TEST(JsonReader, ReportsBadTypePath)
{
	const nlohmann::json json = {{"count", "many"}};
	const pg::JsonReader reader(json, "sample.json");
	expectJsonErrorAt([&reader]() {
		(void)reader.require<int>("count");
	},
					  "$.count");
}

TEST(JsonReader, ReportsBadEnumPath)
{
	const nlohmann::json json = {{"mode", "unknown"}};
	const pg::JsonReader reader(json, "sample.json");
	expectJsonErrorAt(
		[&reader]() {
			(void)reader.requireEnum<TestMode>("mode", std::map<std::string, TestMode>{{"fast", TestMode::Fast}});
		},
		"$.mode");
}

TEST(GameRules, ParsesValidSample)
{
	const nlohmann::json json = validGameRulesJson();
	pg::JsonReader reader(json, "sample.json");
	const pg::GameRules rules = pg::parseGameRules(reader);

	EXPECT_DOUBLE_EQ(rules.maxVerticalTraversalGap, 0.5);
	EXPECT_EQ(rules.overlayMasks.at("hovered"), (std::array<int, 2>{2, 1}));
}

TEST(GameRules, RejectsUnknownField)
{
	nlohmann::json json = validGameRulesJson();
	json["unexpected"] = true;
	pg::JsonReader reader(json, "sample.json");
	expectJsonErrorAt([&reader]() {
		(void)pg::parseGameRules(reader);
	},
					  "$.unexpected");
}
