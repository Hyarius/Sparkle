#include <gtest/gtest.h>

#include "abilities/targeting_definition.hpp"
#include "support/json_fixture.hpp"

#include <string>
#include <string_view>

namespace
{
	[[nodiscard]] pg::TargetFilter parseFilter(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseTargetFilter(reader);
	}

	[[nodiscard]] pg::RangeDefinition parseRange(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseRangeDefinition(reader);
	}

	[[nodiscard]] pg::AreaDefinition parseArea(std::string_view p_content)
	{
		const pgtest::Document document(p_content);
		pg::JsonReader reader = document.reader();
		return pg::parseAreaDefinition(reader);
	}

	[[nodiscard]] std::string filter(
		std::string_view p_self,
		std::string_view p_allies,
		std::string_view p_enemies,
		std::string_view p_defeated,
		std::string_view p_emptyCell)
	{
		return std::string(R"({"allowSelf": )") + std::string(p_self) + R"(, "allowAllies": )" +
			   std::string(p_allies) + R"(, "allowEnemies": )" + std::string(p_enemies) + R"(, "allowDefeated": )" +
			   std::string(p_defeated) + R"(, "allowEmptyCell": )" + std::string(p_emptyCell) + "}";
	}
}

TEST(TargetFilterTest, ReadsEveryRelationExplicitly)
{
	const pg::TargetFilter enemyOnly = parseFilter(filter("false", "false", "true", "false", "false"));
	EXPECT_FALSE(enemyOnly.allowSelf);
	EXPECT_FALSE(enemyOnly.allowAllies);
	EXPECT_TRUE(enemyOnly.allowEnemies);
	EXPECT_FALSE(enemyOnly.allowDefeated);
	EXPECT_FALSE(enemyOnly.allowEmptyCell);

	const pg::TargetFilter friendly = parseFilter(filter("true", "true", "false", "false", "false"));
	EXPECT_TRUE(friendly.allowSelf);
	EXPECT_TRUE(friendly.allowAllies);
	EXPECT_FALSE(friendly.allowEnemies);

	const pg::TargetFilter emptyCell = parseFilter(filter("false", "false", "false", "false", "true"));
	EXPECT_TRUE(emptyCell.allowEmptyCell);
}

TEST(TargetFilterTest, RequiresAllFiveBooleansAndRejectsAnyExtra)
{
	EXPECT_THROW(
		(void)parseFilter(R"({"allowSelf": false, "allowAllies": false, "allowEnemies": true, "allowDefeated": false})"),
		pg::JsonError)
		<< "friendly fire is never inferred: allowEmptyCell must be authored";

	EXPECT_THROW(
		(void)parseFilter(
			R"({"allowSelf": false, "allowAllies": false, "allowEnemies": true, "allowDefeated": false,
			   "allowEmptyCell": false, "allowNeutrals": true})"),
		pg::JsonError);

	EXPECT_THROW((void)parseFilter(filter("false", "false", R"("yes")", "false", "false")), pg::JsonError)
		<< "a relation is a boolean, not a string";
}

TEST(TargetFilterTest, RejectsTheReservedDefeatedRelationAndAFilterThatAllowsNothing)
{
	EXPECT_THROW((void)parseFilter(filter("false", "false", "true", "true", "false")), pg::JsonError)
		<< "allowDefeated is reserved for a future revive schema";

	EXPECT_THROW((void)parseFilter(filter("false", "false", "false", "false", "false")), pg::JsonError)
		<< "a filter that allows nothing can never select a target";
}

TEST(RangeDefinitionTest, ReadsEveryShape)
{
	EXPECT_EQ(
		parseRange(R"({"shape": "self", "minimum": 0, "maximum": 0, "requiresLineOfSight": false})").shape,
		pg::RangeShape::Self);
	EXPECT_EQ(
		parseRange(R"({"shape": "diamond", "minimum": 1, "maximum": 4, "requiresLineOfSight": true})").shape,
		pg::RangeShape::Diamond);
	EXPECT_EQ(
		parseRange(R"({"shape": "orthogonalLine", "minimum": 1, "maximum": 6, "requiresLineOfSight": true})").shape,
		pg::RangeShape::OrthogonalLine);
	EXPECT_EQ(
		parseRange(R"({"shape": "diagonalLine", "minimum": 2, "maximum": 8, "requiresLineOfSight": false})").shape,
		pg::RangeShape::DiagonalLine);

	const pg::RangeDefinition range =
		parseRange(R"({"shape": "diamond", "minimum": 1, "maximum": 4, "requiresLineOfSight": true})");
	EXPECT_EQ(range.minimum, 1);
	EXPECT_EQ(range.maximum, 4);
	EXPECT_TRUE(range.requiresLineOfSight);
}

TEST(RangeDefinitionTest, RejectsUnknownShapesListingTheKnownOnes)
{
	try
	{
		(void)parseRange(R"({"shape": "cone", "minimum": 1, "maximum": 4, "requiresLineOfSight": true})");
		FAIL() << "a cone is not a range shape";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.path(), "$.shape");
		for (const std::string_view known : {"self", "diamond", "orthogonalLine", "diagonalLine"})
		{
			EXPECT_NE(error.message().find(known), std::string::npos) << "should list '" << known << "'";
		}
	}
}

TEST(RangeDefinitionTest, RejectsDistancesOutsideTheirBoundsOrOrder)
{
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "diamond", "minimum": -1, "maximum": 4, "requiresLineOfSight": true})"),
		pg::JsonError);
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "diamond", "minimum": 1, "maximum": 65, "requiresLineOfSight": true})"),
		pg::JsonError);
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "diamond", "minimum": 5, "maximum": 4, "requiresLineOfSight": true})"),
		pg::JsonError)
		<< "a minimum above the maximum reaches no cell";

	EXPECT_EQ(
		parseRange(R"({"shape": "diamond", "minimum": 64, "maximum": 64, "requiresLineOfSight": true})").maximum,
		64);
}

TEST(RangeDefinitionTest, SelfAnchorsOnTheSourceCellAndNothingElseMayReachZero)
{
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "self", "minimum": 0, "maximum": 1, "requiresLineOfSight": false})"),
		pg::JsonError)
		<< "a self range must be exactly the source cell";
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "self", "minimum": 1, "maximum": 1, "requiresLineOfSight": false})"),
		pg::JsonError);
	EXPECT_THROW(
		(void)parseRange(R"({"shape": "diamond", "minimum": 0, "maximum": 0, "requiresLineOfSight": false})"),
		pg::JsonError)
		<< "a non-self range that reaches nowhere should be authored as 'self'";
}

TEST(AreaDefinitionTest, ReadsEveryShape)
{
	EXPECT_EQ(parseArea(R"({"shape": "single", "radius": 0})").shape, pg::AreaShape::Single);
	EXPECT_EQ(parseArea(R"({"shape": "diamond", "radius": 2})").shape, pg::AreaShape::Diamond);
	EXPECT_EQ(parseArea(R"({"shape": "square", "radius": 1})").shape, pg::AreaShape::Square);
	EXPECT_EQ(parseArea(R"({"shape": "cross", "radius": 3})").shape, pg::AreaShape::Cross);
	EXPECT_EQ(parseArea(R"({"shape": "line", "radius": 4})").shape, pg::AreaShape::Line);

	EXPECT_EQ(parseArea(R"({"shape": "diamond", "radius": 2})").radius, 2);
}

TEST(AreaDefinitionTest, RejectsARadiusOutsideItsBoundsOrOnASingleCell)
{
	EXPECT_THROW((void)parseArea(R"({"shape": "single", "radius": 1})"), pg::JsonError)
		<< "a single-cell area has no radius";
	EXPECT_THROW((void)parseArea(R"({"shape": "diamond", "radius": -1})"), pg::JsonError);
	EXPECT_THROW((void)parseArea(R"({"shape": "diamond", "radius": 17})"), pg::JsonError);
	EXPECT_THROW((void)parseArea(R"({"shape": "diamond", "radius": 2, "falloff": 1})"), pg::JsonError);

	// The other shapes accept zero as a degenerate single cell.
	EXPECT_EQ(parseArea(R"({"shape": "cross", "radius": 0})").radius, 0);
	EXPECT_EQ(parseArea(R"({"shape": "diamond", "radius": 16})").radius, 16);
}
