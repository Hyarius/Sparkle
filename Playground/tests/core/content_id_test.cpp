#include <gtest/gtest.h>

#include "core/content_id.hpp"
#include "core/json.hpp"

#include <string>

TEST(ContentIdTest, AcceptsTheKebabGrammar)
{
	EXPECT_TRUE(pg::isContentId("a"));
	EXPECT_TRUE(pg::isContentId("0"));
	EXPECT_TRUE(pg::isContentId("root"));
	EXPECT_TRUE(pg::isContentId("fire-bolt"));
	EXPECT_TRUE(pg::isContentId("gym-leader-2"));
	EXPECT_TRUE(pg::isContentId("a-b-c-d-e"));
	EXPECT_TRUE(pg::isContentId("v2"));
}

TEST(ContentIdTest, RejectsEverythingOutsideTheGrammar)
{
	EXPECT_FALSE(pg::isContentId(""));
	EXPECT_FALSE(pg::isContentId("Fire-Bolt"));
	EXPECT_FALSE(pg::isContentId("fire_bolt"));
	EXPECT_FALSE(pg::isContentId("fire bolt"));
	EXPECT_FALSE(pg::isContentId("fire.bolt"));
	EXPECT_FALSE(pg::isContentId("fire/bolt"));
	EXPECT_FALSE(pg::isContentId("-fire"));
	EXPECT_FALSE(pg::isContentId("fire-"));
	EXPECT_FALSE(pg::isContentId("fire--bolt"));
	EXPECT_FALSE(pg::isContentId("-"));
	EXPECT_FALSE(pg::isContentId("fire\tbolt"));
	EXPECT_FALSE(pg::isContentId("café"));
}

TEST(ContentIdTest, LengthBoundaryIsSixtyFourBytes)
{
	EXPECT_TRUE(pg::isContentId(std::string(pg::MaxContentIdLength, 'a')));
	EXPECT_FALSE(pg::isContentId(std::string(pg::MaxContentIdLength + 1, 'a')));
	EXPECT_TRUE(pg::isContentId(std::string(1, 'a')));
}

TEST(ContentIdTest, RequireReportsTheFileThePathAndTheGrammar)
{
	EXPECT_NO_THROW(pg::requireContentId("root", "feats/starter.json", "$.nodes[0].id", "feat node id"));

	try
	{
		pg::requireContentId("Root Node", "feats/starter.json", "$.nodes[0].id", "feat node id");
		FAIL() << "an out-of-grammar id must be rejected";
	} catch (const pg::JsonError &error)
	{
		EXPECT_EQ(error.file(), std::filesystem::path("feats/starter.json"));
		EXPECT_EQ(error.path(), "$.nodes[0].id");
		EXPECT_NE(error.message().find("feat node id"), std::string::npos);
		EXPECT_NE(error.message().find("Root Node"), std::string::npos);
		EXPECT_NE(error.message().find("[a-z0-9]"), std::string::npos);
	}
}
