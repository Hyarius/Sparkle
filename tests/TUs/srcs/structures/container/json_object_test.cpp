#include "structures/container/spk_json_object.hpp"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>

#include <gtest/gtest.h>

namespace application_model
{
	using spk::JSON::FormatOptions;
	using spk::JSON::Object;

	struct StaticConfig
	{
		std::string name;
		std::int32_t width = 0;
		std::int32_t height = 0;
		bool fullscreen = false;
	};

	Object toJSON(const StaticConfig &p_config)
	{
		Object result = Object::object();
		result["name"] = p_config.name;
		result["width"] = p_config.width;
		result["height"] = p_config.height;
		result["fullscreen"] = p_config.fullscreen;
		return result;
	}

	void fromJSON(const Object &p_json, StaticConfig &p_config)
	{
		p_config.name = p_json.at("name").as<std::string>();
		p_config.width = p_json.at("width").as<std::int32_t>();
		p_config.height = p_json.at("height").as<std::int32_t>();
		p_config.fullscreen = p_json.at("fullscreen").as<bool>();
	}

	struct MemberConfig
	{
		std::string label;
		std::int32_t value = 0;
	};

	Object toJSON(const MemberConfig &p_config)
	{
		Object result = Object::object();
		result["label"] = p_config.label;
		result["value"] = p_config.value;
		return result;
	}

	void fromJSON(const Object &p_json, MemberConfig &p_config)
	{
		p_config.label = p_json.at("label").as<std::string>();
		p_config.value = p_json.at("value").as<std::int32_t>();
	}
}

namespace
{
	using application_model::MemberConfig;
	using application_model::StaticConfig;
	using spk::JSON::FormatOptions;
	using spk::JSON::Object;

	static_assert(spk::JSON::json_writable<StaticConfig>);
	static_assert(spk::JSON::json_readable<StaticConfig>);

	static_assert(spk::JSON::json_writable<MemberConfig>);
	static_assert(spk::JSON::json_readable<MemberConfig>);
}

TEST(SPKJsonObject, DefaultConstructsAsNull)
{
	Object value;

	EXPECT_EQ(value.type(), Object::Type::Null);
	EXPECT_TRUE(value.isNull());
	EXPECT_TRUE(value.holds<std::nullptr_t>());
	EXPECT_EQ(value.as<std::nullptr_t>(), nullptr);
}

TEST(SPKJsonObject, StoresNativeScalarValues)
{
	Object boolean = true;
	Object integer = std::int32_t{42};
	Object floating = 3.5f;
	Object string = std::string("Sparkle");

	EXPECT_TRUE(boolean.isBoolean());
	EXPECT_TRUE(integer.isInteger());
	EXPECT_TRUE(floating.isFloating());
	EXPECT_TRUE(string.isString());

	EXPECT_TRUE(boolean.as<bool>());
	EXPECT_EQ(integer.as<std::int32_t>(), 42);
	EXPECT_EQ(integer.as<std::int64_t>(), 42);
	EXPECT_DOUBLE_EQ(floating.as<double>(), 3.5);
	EXPECT_EQ(string.as<std::string>(), "Sparkle");
}

TEST(SPKJsonObject, AcceptsStringViewAndStringLiteral)
{
	Object first = "literal";
	Object second = std::string_view("view");

	EXPECT_EQ(first.as<std::string>(), "literal");
	EXPECT_EQ(second.as<std::string>(), "view");
}

TEST(SPKJsonObject, ConvertsIntegerToFloatingOnRead)
{
	Object value = 42;

	EXPECT_TRUE(value.holds<double>());
	EXPECT_DOUBLE_EQ(value.as<double>(), 42.0);
}

TEST(SPKJsonObject, RejectsWrongNativeTypeOnRead)
{
	Object value = "not an integer";

	EXPECT_FALSE(value.holds<std::int32_t>());
	EXPECT_ANY_THROW((void)value.as<std::int32_t>());
}

TEST(SPKJsonObject, RejectsIntegerReadOutsideRequestedRange)
{
	Object value = std::numeric_limits<std::int64_t>::max();

	EXPECT_ANY_THROW((void)value.as<std::int32_t>());
}

TEST(SPKJsonObject, RejectsUnsignedReadOfNegativeValue)
{
	Object value = -1;

	EXPECT_ANY_THROW((void)value.as<std::uint32_t>());
}

TEST(SPKJsonObject, NullSubscriptCreatesObject)
{
	Object root;

	root["name"] = "Sparkle";
	root["version"] = 1;

	EXPECT_TRUE(root.isObject());
	EXPECT_TRUE(root.contains("name"));
	EXPECT_TRUE(root.contains("version"));
	EXPECT_EQ(root.size(), 2u);
	EXPECT_EQ(root.at("name").as<std::string>(), "Sparkle");
	EXPECT_EQ(root.at("version").as<int>(), 1);
}

TEST(SPKJsonObject, ConstSubscriptDoesNotCreateMissingMember)
{
	Object root = Object::object();
	root["existing"] = true;

	const Object &constRoot = root;

	EXPECT_TRUE(constRoot["existing"].as<bool>());
	EXPECT_ANY_THROW((void)constRoot["missing"]);
}

TEST(SPKJsonObject, FindReturnsNullWhenMemberIsMissing)
{
	Object root = Object::object();
	root["existing"] = 123;

	EXPECT_NE(root.find("existing"), nullptr);
	EXPECT_EQ(root.find("missing"), nullptr);
}

TEST(SPKJsonObject, AtThrowsWhenMemberIsMissing)
{
	Object root = Object::object();

	EXPECT_ANY_THROW((void)root.at("missing"));
}

TEST(SPKJsonObject, ArrayAppendPushBackResizeAndIndexAccess)
{
	Object array = Object::array();

	array.append() = "first";
	array.pushBack(Object(2));
	array.pushBack(3);

	ASSERT_TRUE(array.isArray());
	ASSERT_EQ(array.size(), 3u);
	EXPECT_EQ(array[0].as<std::string>(), "first");
	EXPECT_EQ(array[1].as<int>(), 2);
	EXPECT_EQ(array[2].as<int>(), 3);

	array.resize(5);
	EXPECT_EQ(array.size(), 5u);
	EXPECT_TRUE(array[3].isNull());
	EXPECT_TRUE(array[4].isNull());
}

TEST(SPKJsonObject, NullAppendCreatesArray)
{
	Object value;

	value.append() = 10;
	value.append() = 20;

	EXPECT_TRUE(value.isArray());
	ASSERT_EQ(value.size(), 2u);
	EXPECT_EQ(value.at(0).as<int>(), 10);
	EXPECT_EQ(value.at(1).as<int>(), 20);
}

TEST(SPKJsonObject, ArrayIndexOutOfRangeThrows)
{
	Object array = Object::array();
	array.append() = 1;

	EXPECT_ANY_THROW((void)array.at(1));
	EXPECT_ANY_THROW((void)array[1]);
}

TEST(SPKJsonObject, ObjectOperationsThrowOnNonObject)
{
	Object value = 12;

	EXPECT_ANY_THROW((void)value.contains("key"));
	EXPECT_ANY_THROW((void)value.find("key"));
	EXPECT_ANY_THROW((void)value.at("key"));
}

TEST(SPKJsonObject, ArrayOperationsThrowOnNonArray)
{
	Object value = Object::object();

	EXPECT_ANY_THROW((void)value.at(0));
	EXPECT_ANY_THROW((void)value[0]);
}

TEST(SPKJsonObject, SerializesCompactJSON)
{
	Object root = Object::object();
	root["active"] = true;
	root["name"] = "Sparkle";
	root["version"] = 1;

	const std::string content = root.toString(FormatOptions{.pretty = false});

	// std::map orders keys lexicographically.
	EXPECT_EQ(content, R"({"active":true,"name":"Sparkle","version":1})");
}

TEST(SPKJsonObject, SerializesPrettyJSON)
{
	Object root = Object::object();
	root["name"] = "Sparkle";
	root["version"] = 1;

	const std::string content = root.toString(FormatOptions{.pretty = true, .indentationSize = 2});

	EXPECT_NE(content.find("\n"), std::string::npos);
	EXPECT_NE(content.find("  \"name\": \"Sparkle\""), std::string::npos);
	EXPECT_NE(content.find("  \"version\": 1"), std::string::npos);
}

TEST(SPKJsonObject, EscapesSerializedStrings)
{
	Object value = std::string("line\nquote\"slash\\tab\t");

	EXPECT_EQ(value.toString(FormatOptions{.pretty = false}), R"("line\nquote\"slash\\tab\t")");
}

TEST(SPKJsonObject, ParsesScalars)
{
	EXPECT_TRUE(Object::fromString("null").isNull());
	EXPECT_TRUE(Object::fromString("true").as<bool>());
	EXPECT_FALSE(Object::fromString("false").as<bool>());
	EXPECT_EQ(Object::fromString("-42").as<int>(), -42);
	EXPECT_DOUBLE_EQ(Object::fromString("1.25").as<double>(), 1.25);
	EXPECT_DOUBLE_EQ(Object::fromString("1e2").as<double>(), 100.0);
	EXPECT_EQ(Object::fromString(R"("Sparkle")").as<std::string>(), "Sparkle");
}

TEST(SPKJsonObject, ParsesObjectsAndArrays)
{
	Object root = Object::fromString(R"({"name":"Sparkle","values":[1,2,3],"nested":{"enabled":true}})");

	ASSERT_TRUE(root.isObject());
	EXPECT_EQ(root.at("name").as<std::string>(), "Sparkle");

	const Object &values = root.at("values");
	ASSERT_TRUE(values.isArray());
	ASSERT_EQ(values.size(), 3u);
	EXPECT_EQ(values.at(0).as<int>(), 1);
	EXPECT_EQ(values.at(1).as<int>(), 2);
	EXPECT_EQ(values.at(2).as<int>(), 3);

	EXPECT_TRUE(root.at("nested").at("enabled").as<bool>());
}

TEST(SPKJsonObject, ParsesEscapedStrings)
{
	Object value = Object::fromString(R"("line\nquote\"slash\\tab\t")");

	EXPECT_EQ(value.as<std::string>(), "line\nquote\"slash\\tab\t");
}

TEST(SPKJsonObject, ParsesUnicodeEscapes)
{
	Object ascii = Object::fromString(R"("A\u0042C")");
	Object utf8 = Object::fromString(R"("\u00E9 \uD83D\uDE03")");

	EXPECT_EQ(ascii.as<std::string>(), "ABC");
	EXPECT_EQ(utf8.as<std::string>(), "\xC3\xA9 \xF0\x9F\x98\x83");
}

TEST(SPKJsonObject, PreservesUTF8StringsDuringRoundTrip)
{
	const std::string text = "caf\xC3\xA9 \xF0\x9F\x98\x83";
	Object parsed = Object::fromString(Object(text).toString(FormatOptions{.pretty = false}));

	EXPECT_EQ(parsed.as<std::string>(), text);
}

TEST(SPKJsonObject, RejectsMalformedJSON)
{
	EXPECT_ANY_THROW((void)Object::fromString(""));
	EXPECT_ANY_THROW((void)Object::fromString(R"({"a":})"));
	EXPECT_ANY_THROW((void)Object::fromString(R"([1,2,)"));
	EXPECT_ANY_THROW((void)Object::fromString(R"({"a":1,"a":2})"));
	EXPECT_ANY_THROW((void)Object::fromString("01"));
	EXPECT_ANY_THROW((void)Object::fromString(R"("unterminated)"));
	EXPECT_ANY_THROW((void)Object::fromString(R"("\x")"));
}

TEST(SPKJsonObject, RoundTripsThroughString)
{
	Object root = Object::object();
	root["name"] = "Sparkle";
	root["numbers"] = Object::array();
	root["numbers"].append() = 1;
	root["numbers"].append() = 2;
	root["numbers"].append() = 3;
	root["enabled"] = true;

	Object parsed = Object::fromString(root.toString(FormatOptions{.pretty = false}));

	EXPECT_EQ(parsed.at("name").as<std::string>(), "Sparkle");
	EXPECT_TRUE(parsed.at("enabled").as<bool>());
	ASSERT_EQ(parsed.at("numbers").size(), 3u);
	EXPECT_EQ(parsed.at("numbers").at(0).as<int>(), 1);
	EXPECT_EQ(parsed.at("numbers").at(1).as<int>(), 2);
	EXPECT_EQ(parsed.at("numbers").at(2).as<int>(), 3);
}

TEST(SPKJsonObject, LoadsAndSavesFile)
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "spk_json_object_tests.json";

	Object root = Object::object();
	root["name"] = "Sparkle";
	root["version"] = 1;
	root.saveToFile(path, FormatOptions{.pretty = false});

	Object loaded = Object::loadFromFile(path);

	EXPECT_EQ(loaded.at("name").as<std::string>(), "Sparkle");
	EXPECT_EQ(loaded.at("version").as<int>(), 1);

	std::filesystem::remove(path);
}

TEST(SPKJsonObject, FreeFunctionSerializableTypeCanBeAssignedAndReadBack)
{
	StaticConfig config{
		.name = "Game",
		.width = 1920,
		.height = 1080,
		.fullscreen = true};

	Object root = Object::object();
	root["config"] = config;

	StaticConfig loaded = root.at("config").as<StaticConfig>();

	EXPECT_EQ(loaded.name, "Game");
	EXPECT_EQ(loaded.width, 1920);
	EXPECT_EQ(loaded.height, 1080);
	EXPECT_TRUE(loaded.fullscreen);
}

TEST(SPKJsonObject, ExternalTypeCanOptInThroughADLFunctions)
{
	MemberConfig config{
		.label = "Health",
		.value = 100};

	Object root = Object::object();
	root["config"] = config;

	MemberConfig loaded = root.at("config").as<MemberConfig>();

	EXPECT_EQ(loaded.label, "Health");
	EXPECT_EQ(loaded.value, 100);
}

TEST(SPKJsonObject, ResetReturnsToNull)
{
	Object value = Object::object();
	value["name"] = "Sparkle";

	value.reset();

	EXPECT_TRUE(value.isNull());
	EXPECT_EQ(value.type(), Object::Type::Null);
}
