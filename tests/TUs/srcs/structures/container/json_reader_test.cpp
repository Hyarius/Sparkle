#include "structures/container/spk_json_reader.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
	using spk::JSON::Error;
	using spk::JSON::Reader;
	using spk::JSON::Value;

	const std::filesystem::path kFile = "data/sample.json";

	[[nodiscard]] Value parse(const char *p_content)
	{
		return Value::fromString(p_content);
	}
}

TEST(JsonReader, ReadsRequiredScalarsAndSequences)
{
	const Value document = parse(R"({
		"name": "tower",
		"height": 12,
		"solid": true,
		"tags": ["stone", "old"],
		"cell": [3, 4]
	})");
	const Reader reader(document, kFile);

	EXPECT_EQ(reader.require<std::string>("name"), "tower");
	EXPECT_EQ(reader.require<std::int32_t>("height"), 12);
	EXPECT_EQ(reader.require<bool>("solid"), true);
	EXPECT_EQ(reader.require<std::vector<std::string>>("tags"), (std::vector<std::string>{"stone", "old"}));
	EXPECT_EQ((reader.require<std::array<int, 2>>("cell")), (std::array<int, 2>{3, 4}));
}

TEST(JsonReader, MissingRequiredFieldNamesTheExactPath)
{
	const Value document = parse(R"({"present": 1})");
	const Reader reader(document, kFile);

	try
	{
		(void)reader.require<int>("absent");
		FAIL() << "require should have thrown";
	} catch (const Error &error)
	{
		EXPECT_EQ(error.file(), kFile);
		EXPECT_EQ(error.path(), "$.absent");
		EXPECT_EQ(error.message(), "missing required field");
	}
}

TEST(JsonReader, TypeMismatchesAndWrongArityAreWrappedWithContext)
{
	const Value document = parse(R"({"count": "three", "cell": [1, 2, 3]})");
	const Reader reader(document, kFile);

	EXPECT_THROW((void)reader.require<int>("count"), Error);
	// Fixed arrays demand the exact element count.
	EXPECT_THROW((void)(reader.require<std::array<int, 2>>("cell")), Error);
	// Reading a member of a non-object fails up front.
	const Value scalar = parse("42");
	const Reader scalarReader(scalar, kFile);
	EXPECT_THROW((void)scalarReader.contains("anything"), Error);
}

TEST(JsonReader, OptionalFallsBackOnlyWhenAbsent)
{
	const Value document = parse(R"({"present": 7, "wrongType": []})");
	const Reader reader(document, kFile);

	EXPECT_EQ(reader.optional<int>("present", 3), 7);
	EXPECT_EQ(reader.optional<int>("absent", 3), 3);
	// Present-but-invalid must fail loudly rather than silently defaulting.
	EXPECT_THROW((void)reader.optional<int>("wrongType", 3), Error);
}

TEST(JsonReader, RequireEnumListsTheKnownValues)
{
	const Value document = parse(R"({"traversal": "swimming"})");
	const Reader reader(document, kFile);
	const std::map<std::string, int> values = {{"passable", 0}, {"solid", 1}};

	try
	{
		(void)reader.requireEnum<int>("traversal", values);
		FAIL() << "requireEnum should have thrown";
	} catch (const Error &error)
	{
		EXPECT_EQ(error.path(), "$.traversal");
		EXPECT_NE(error.message().find("passable, solid"), std::string::npos);
	}

	const Value valid = parse(R"({"traversal": "solid"})");
	EXPECT_EQ((Reader(valid, kFile).requireEnum<int>("traversal", values)), 1);
}

TEST(JsonReader, ChildReadersExtendThePath)
{
	const Value document = parse(R"({
		"shape": {"type": "cube"},
		"anchors": [{"name": "door"}, {"name": "exit"}]
	})");
	const Reader reader(document, kFile);

	const Reader shape = reader.child("shape");
	EXPECT_EQ(shape.path(), "$.shape");
	EXPECT_EQ(shape.require<std::string>("type"), "cube");

	const std::vector<Reader> anchors = reader.childArray("anchors");
	ASSERT_EQ(anchors.size(), 2u);
	EXPECT_EQ(anchors[1].path(), "$.anchors[1]");
	EXPECT_EQ(anchors[1].require<std::string>("name"), "exit");

	// Wrong shapes are rejected with the child's path.
	EXPECT_THROW((void)reader.child("anchors"), Error);
	EXPECT_THROW((void)reader.childArray("shape"), Error);
}

TEST(JsonReader, ForbidUnknownRejectsUnexpectedKeys)
{
	const Value document = parse(R"({"version": 1, "typo": true})");
	const Reader reader(document, kFile);

	EXPECT_NO_THROW(reader.forbidUnknown({"version", "typo"}));
	try
	{
		reader.forbidUnknown({"version"});
		FAIL() << "forbidUnknown should have thrown";
	} catch (const Error &error)
	{
		EXPECT_EQ(error.path(), "$.typo");
		EXPECT_EQ(error.message(), "unknown field");
	}
}

TEST(JsonLoader, ParseFileWrapsFailuresAsJsonErrors)
{
	try
	{
		(void)spk::JSON::Loader::parseFile("does/not/exist.json");
		FAIL() << "parseFile should have thrown";
	} catch (const Error &error)
	{
		EXPECT_EQ(error.file(), std::filesystem::path("does/not/exist.json"));
		EXPECT_EQ(error.path(), "$");
		EXPECT_NE(error.message().find("invalid JSON"), std::string::npos);
	}
}
