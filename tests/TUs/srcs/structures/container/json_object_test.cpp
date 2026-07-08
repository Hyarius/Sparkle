#include "structures/container/spk_json_object.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>

#include <gtest/gtest.h>

namespace application_model
{
	using spk::JSON::FormatOptions;
	using spk::JSON::Object;
	using spk::JSON::ParseOptions;
	using spk::JSON::Value;

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

using spk::JSON::FormatOptions;
using spk::JSON::ParseOptions;
using spk::JSON::Value;

TEST(SPKJsonValueCompatibility, ObjectAliasWorks)
{
	static_assert(std::same_as<spk::JSON::Object, spk::JSON::Value>);

	spk::JSON::Object value = spk::JSON::Object::object();
	value["name"] = "Sparkle";

	EXPECT_EQ(value.at("name").as<std::string>(), "Sparkle");
}

TEST(SPKJsonValue, RejectsUnsignedIntegerTooLargeForInt64Storage)
{
	const auto tooLarge = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1u;

	EXPECT_ANY_THROW((void)Value(tooLarge));

	Value value = 42;
	EXPECT_ANY_THROW(value = tooLarge);
	EXPECT_EQ(value.as<int>(), 42);
}

TEST(SPKJsonValue, RejectsNonFiniteFloatingValues)
{
	EXPECT_ANY_THROW((void)Value(std::numeric_limits<double>::quiet_NaN()));
	EXPECT_ANY_THROW((void)Value(std::numeric_limits<double>::infinity()));
	EXPECT_ANY_THROW((void)Value(-std::numeric_limits<double>::infinity()));

	Value value;
	EXPECT_ANY_THROW(value = std::numeric_limits<double>::quiet_NaN());
	EXPECT_TRUE(value.isNull());
}

TEST(SPKJsonValue, SerializesFloatingValuesIndependentlyOfStreamLocale)
{
	class CommaDecimalPoint : public std::numpunct<char>
	{
	protected:
		char do_decimal_point() const override
		{
			return ',';
		}
	};

	std::ostringstream stream;
	stream.imbue(std::locale(std::locale::classic(), new CommaDecimalPoint));
	Value(3.25).write(stream, FormatOptions{.pretty = false});

	EXPECT_EQ(stream.str(), "3.25");
	EXPECT_DOUBLE_EQ(Value::fromString(stream.str()).as<double>(), 3.25);
}

TEST(SPKJsonValue, CanAsChecksIntegerRange)
{
	Value large = 1000000;
	Value negative = -1;

	EXPECT_FALSE(large.canAs<std::uint8_t>());
	EXPECT_FALSE(large.holds<std::uint8_t>());
	EXPECT_FALSE(negative.canAs<std::uint32_t>());
	EXPECT_TRUE(large.canAs<std::int32_t>());
}

TEST(SPKJsonValue, AsReturnsTheRequestedCleanType)
{
	Value value = "Sparkle";

	static_assert(std::same_as<decltype(value.as<const std::string &>()), std::string>);
	EXPECT_EQ(value.as<const std::string &>(), "Sparkle");
}

TEST(SPKJsonValue, Equality)
{
	EXPECT_EQ(Value::fromString(R"({"a":[1,true]})"), Value::fromString(R"({"a":[1,true]})"));
	EXPECT_NE(Value(1), Value(2));
}

TEST(SPKJsonValue, PushBackReturnsInsertedValue)
{
	Value values;
	Value &inserted = values.pushBack(Value::object());
	inserted["name"] = "Sword";

	ASSERT_EQ(values.size(), 1u);
	EXPECT_EQ(values.at(0).at("name").as<std::string>(), "Sword");
}

TEST(SPKJsonValue, ParseOptionsControlDuplicatesBomAndDepth)
{
	const ParseOptions keepLast{.rejectDuplicateKeys = false};
	EXPECT_EQ(Value::fromString(R"({"value":1,"value":2})", keepLast).at("value").as<std::int64_t>(), 2);

	const std::string bomJson = "\xEF\xBB\xBF{\"value\":1}";
	EXPECT_EQ(Value::fromString(bomJson).at("value").as<std::int64_t>(), 1);
	EXPECT_ANY_THROW((void)Value::fromString(
		bomJson, ParseOptions{.rejectDuplicateKeys = true, .allowUtf8Bom = false}));

	EXPECT_ANY_THROW((void)Value::fromString(
		R"({"level":{"tooDeep":true}})",
		ParseOptions{.rejectDuplicateKeys = true, .allowUtf8Bom = true, .maxDepth = 1}));
	EXPECT_NO_THROW((void)Value::fromString(
		R"({"level":true})",
		ParseOptions{.rejectDuplicateKeys = true, .allowUtf8Bom = true, .maxDepth = 1}));
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

TEST(SPKJsonValue, DefaultConstructsAsNull)
{
	Object value;

	EXPECT_EQ(value.type(), Object::Type::Null);
	EXPECT_TRUE(value.isNull());
	EXPECT_TRUE(value.holds<std::nullptr_t>());
	EXPECT_EQ(value.as<std::nullptr_t>(), nullptr);
}

TEST(SPKJsonValue, StoresNativeScalarValues)
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

TEST(SPKJsonValue, AcceptsStringViewAndStringLiteral)
{
	Object first = "literal";
	Object second = std::string_view("view");

	EXPECT_EQ(first.as<std::string>(), "literal");
	EXPECT_EQ(second.as<std::string>(), "view");
}

TEST(SPKJsonValue, ConvertsIntegerToFloatingOnRead)
{
	Object value = 42;

	EXPECT_TRUE(value.holds<double>());
	EXPECT_DOUBLE_EQ(value.as<double>(), 42.0);
}

TEST(SPKJsonValue, RejectsWrongNativeTypeOnRead)
{
	Object value = "not an integer";

	EXPECT_FALSE(value.holds<std::int32_t>());
	EXPECT_ANY_THROW((void)value.as<std::int32_t>());
}

TEST(SPKJsonValue, RejectsIntegerReadOutsideRequestedRange)
{
	Object value = std::numeric_limits<std::int64_t>::max();

	EXPECT_ANY_THROW((void)value.as<std::int32_t>());
}

TEST(SPKJsonValue, RejectsUnsignedReadOfNegativeValue)
{
	Object value = -1;

	EXPECT_ANY_THROW((void)value.as<std::uint32_t>());
}

TEST(SPKJsonValue, NullSubscriptCreatesObject)
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

TEST(SPKJsonValue, ConstSubscriptDoesNotCreateMissingMember)
{
	Object root = Object::object();
	root["existing"] = true;

	const Object &constRoot = root;

	EXPECT_TRUE(constRoot["existing"].as<bool>());
	EXPECT_ANY_THROW((void)constRoot["missing"]);
}

TEST(SPKJsonValue, FindReturnsNullWhenMemberIsMissing)
{
	Object root = Object::object();
	root["existing"] = 123;

	EXPECT_NE(root.find("existing"), nullptr);
	EXPECT_EQ(root.find("missing"), nullptr);
}

TEST(SPKJsonValue, AtThrowsWhenMemberIsMissing)
{
	Object root = Object::object();

	EXPECT_ANY_THROW((void)root.at("missing"));
}

TEST(SPKJsonValue, ArrayAppendPushBackResizeAndIndexAccess)
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

TEST(SPKJsonValue, NullAppendCreatesArray)
{
	Object value;

	value.append() = 10;
	value.append() = 20;

	EXPECT_TRUE(value.isArray());
	ASSERT_EQ(value.size(), 2u);
	EXPECT_EQ(value.at(0).as<int>(), 10);
	EXPECT_EQ(value.at(1).as<int>(), 20);
}

TEST(SPKJsonValue, ArrayIndexOutOfRangeThrows)
{
	Object array = Object::array();
	array.append() = 1;

	EXPECT_ANY_THROW((void)array.at(1));
	EXPECT_ANY_THROW((void)array[1]);
}

TEST(SPKJsonValue, ObjectOperationsThrowOnNonObject)
{
	Object value = 12;

	EXPECT_ANY_THROW((void)value.contains("key"));
	EXPECT_ANY_THROW((void)value.find("key"));
	EXPECT_ANY_THROW((void)value.at("key"));
}

TEST(SPKJsonValue, ArrayOperationsThrowOnNonArray)
{
	Object value = Object::object();

	EXPECT_ANY_THROW((void)value.at(0));
	EXPECT_ANY_THROW((void)value[0]);
}

TEST(SPKJsonValue, SerializesCompactJSON)
{
	Object root = Object::object();
	root["active"] = true;
	root["name"] = "Sparkle";
	root["version"] = 1;

	const std::string content = root.toString(FormatOptions{.pretty = false});

	// std::map orders keys lexicographically.
	EXPECT_EQ(content, R"({"active":true,"name":"Sparkle","version":1})");
}

TEST(SPKJsonValue, SerializesPrettyJSON)
{
	Object root = Object::object();
	root["name"] = "Sparkle";
	root["version"] = 1;

	const std::string content = root.toString(FormatOptions{.pretty = true, .indentationSize = 2});

	EXPECT_NE(content.find("\n"), std::string::npos);
	EXPECT_NE(content.find("  \"name\": \"Sparkle\""), std::string::npos);
	EXPECT_NE(content.find("  \"version\": 1"), std::string::npos);
}

TEST(SPKJsonValue, SerializesEmptyObjectAndArray)
{
	EXPECT_EQ(Value::object().toString(FormatOptions{.pretty = false}), "{}");
	EXPECT_EQ(Value::array().toString(FormatOptions{.pretty = false}), "[]");
}

TEST(SPKJsonValue, EscapesSerializedStrings)
{
	Object value = std::string("line\nquote\"slash\\tab\t");

	EXPECT_EQ(value.toString(FormatOptions{.pretty = false}), R"("line\nquote\"slash\\tab\t")");
}

TEST(SPKJsonValue, ParsesScalars)
{
	EXPECT_TRUE(Object::fromString("null").isNull());
	EXPECT_TRUE(Object::fromString("true").as<bool>());
	EXPECT_FALSE(Object::fromString("false").as<bool>());
	EXPECT_EQ(Object::fromString("-42").as<int>(), -42);
	EXPECT_DOUBLE_EQ(Object::fromString("1.25").as<double>(), 1.25);
	EXPECT_DOUBLE_EQ(Object::fromString("1e2").as<double>(), 100.0);
	EXPECT_EQ(Object::fromString(R"("Sparkle")").as<std::string>(), "Sparkle");
}

TEST(SPKJsonValue, ParsesObjectsAndArrays)
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

TEST(SPKJsonValue, ParsesEscapedStrings)
{
	Object value = Object::fromString(R"("line\nquote\"slash\\tab\t")");

	EXPECT_EQ(value.as<std::string>(), "line\nquote\"slash\\tab\t");
}

TEST(SPKJsonValue, ParsesUnicodeEscapes)
{
	Object ascii = Object::fromString(R"("A\u0042C")");
	Object utf8 = Object::fromString(R"("\u00E9 \uD83D\uDE03")");

	EXPECT_EQ(ascii.as<std::string>(), "ABC");
	EXPECT_EQ(utf8.as<std::string>(), "\xC3\xA9 \xF0\x9F\x98\x83");
}

TEST(SPKJsonValue, RejectsMalformedUnicodeEscapes)
{
	EXPECT_ANY_THROW((void)Value::fromString(R"("\u12G4")"));
	EXPECT_ANY_THROW((void)Value::fromString(R"("\uD83D")"));
	EXPECT_ANY_THROW((void)Value::fromString(R"("\uD83D\u0041")"));
	EXPECT_ANY_THROW((void)Value::fromString(R"("\uDE03")"));
}

TEST(SPKJsonValue, PreservesUTF8StringsDuringRoundTrip)
{
	const std::string text = "caf\xC3\xA9 \xF0\x9F\x98\x83";
	Object parsed = Object::fromString(Object(text).toString(FormatOptions{.pretty = false}));

	EXPECT_EQ(parsed.as<std::string>(), text);
}

TEST(SPKJsonValue, RejectsMalformedJSON)
{
	EXPECT_ANY_THROW((void)Object::fromString(""));
	EXPECT_ANY_THROW((void)Object::fromString(R"({"a":})"));
	EXPECT_ANY_THROW((void)Object::fromString(R"([1,2,)"));
	EXPECT_ANY_THROW((void)Object::fromString(R"({"a":1,"a":2})"));
	EXPECT_ANY_THROW((void)Object::fromString("01"));
	EXPECT_ANY_THROW((void)Object::fromString(R"("unterminated)"));
	EXPECT_ANY_THROW((void)Object::fromString(R"("\x")"));
}

TEST(SPKJsonValue, RejectsTrailingCommas)
{
	EXPECT_ANY_THROW((void)Value::fromString(R"([1,2,])"));
	EXPECT_ANY_THROW((void)Value::fromString(R"({"a":1,})"));
}

TEST(SPKJsonValue, RoundTripsThroughString)
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

TEST(SPKJsonValue, LoadsAndSavesFile)
{
	const std::filesystem::path path = std::filesystem::temp_directory_path() /
		("spk_json_value_tests_" +
		 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");

	Object root = Object::object();
	root["name"] = "Sparkle";
	root["version"] = 1;
	root.saveToFile(path, FormatOptions{.pretty = false});

	Object loaded = Object::loadFromFile(path);

	EXPECT_EQ(loaded.at("name").as<std::string>(), "Sparkle");
	EXPECT_EQ(loaded.at("version").as<int>(), 1);

	std::filesystem::remove(path);
}

TEST(SPKJsonValue, FreeFunctionSerializableTypeCanBeAssignedAndReadBack)
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

TEST(SPKJsonValue, ExternalTypeCanOptInThroughADLFunctions)
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

TEST(SPKJsonValue, ResetReturnsToNull)
{
	Object value = Object::object();
	value["name"] = "Sparkle";

	value.reset();

	EXPECT_TRUE(value.isNull());
	EXPECT_EQ(value.type(), Object::Type::Null);
}
