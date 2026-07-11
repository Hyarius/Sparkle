#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "structures/container/spk_json_reader.hpp"
#include "structures/system/spk_command_line_parser.hpp"

namespace
{
	using Parser = spk::CommandLineParser;
	using Option = spk::CommandLineParser::Option;
}

TEST(CommandLineParserTest, FlagPresenceYieldsTrue)
{
	Parser parser;
	parser.addOption("--map-only", {.type = Option::Type::Flag, .help = "write the map and exit"});

	const char *argv[] = {"prog", "--map-only"};
	const spk::JSON::Value &args = parser.parse(2, argv);

	ASSERT_TRUE(args.contains("map-only"));
	EXPECT_TRUE(args.at("map-only").as<bool>());
}

TEST(CommandLineParserTest, FlagWithoutDefaultStaysAbsentWhenNotPassed)
{
	Parser parser;
	parser.addOption("--map-only", {.type = Option::Type::Flag});

	const char *argv[] = {"prog"};
	const spk::JSON::Value &args = parser.parse(1, argv);

	EXPECT_FALSE(args.contains("map-only"));
}

TEST(CommandLineParserTest, DefaultSeedsKeyWhenOptionIsAbsent)
{
	Parser parser;
	parser.addOption("--size", {.type = Option::Type::Integer, .help = "cell count"}, std::int64_t{124});

	const char *argv[] = {"prog"};
	const spk::JSON::Value &args = parser.parse(1, argv);

	ASSERT_TRUE(args.contains("size"));
	EXPECT_EQ(args.at("size").as<std::int64_t>(), 124);
}

TEST(CommandLineParserTest, SuppliedValueOverridesDefault)
{
	Parser parser;
	parser.addOption("--size", {.type = Option::Type::Integer}, std::int64_t{124});

	const char *argv[] = {"prog", "--size", "8"};
	const spk::JSON::Value &args = parser.parse(3, argv);

	EXPECT_EQ(args.at("size").as<std::int64_t>(), 8);
}

TEST(CommandLineParserTest, ShortNameIsAnAliasForTheSameKey)
{
	Parser parser;
	parser.addOption("--verbose", "-v", {.type = Option::Type::Flag});

	const char *argv[] = {"prog", "-v"};
	const spk::JSON::Value &args = parser.parse(2, argv);

	ASSERT_TRUE(args.contains("verbose"));
	EXPECT_TRUE(args.at("verbose").as<bool>());
}

TEST(CommandLineParserTest, MultipleValuesBecomeAnArray)
{
	Parser parser;
	parser.addOption("--pos", {.type = Option::Type::Integer, .count = 3, .help = "x y z"});

	const char *argv[] = {"prog", "--pos", "1", "2", "3"};
	const spk::JSON::Value &args = parser.parse(5, argv);

	const spk::JSON::Value::Array &pos = args.at("pos").asArray();
	ASSERT_EQ(pos.size(), 3u);
	EXPECT_EQ(pos[0].as<std::int64_t>(), 1);
	EXPECT_EQ(pos[1].as<std::int64_t>(), 2);
	EXPECT_EQ(pos[2].as<std::int64_t>(), 3);
}

TEST(CommandLineParserTest, NegativeIntegerValuePassesThrough)
{
	Parser parser;
	parser.addOption("--offset", {.type = Option::Type::Integer});

	const char *argv[] = {"prog", "--offset", "-5"};
	const spk::JSON::Value &args = parser.parse(3, argv);

	EXPECT_EQ(args.at("offset").as<std::int64_t>(), -5);
}

TEST(CommandLineParserTest, FloatAndStringTypesConvert)
{
	Parser parser;
	parser.addOption("--gain", {.type = Option::Type::Float});
	parser.addOption("--name", {.type = Option::Type::String});

	const char *argv[] = {"prog", "--gain", "1.5", "--name", "world"};
	const spk::JSON::Value &args = parser.parse(5, argv);

	EXPECT_DOUBLE_EQ(args.at("gain").as<double>(), 1.5);
	EXPECT_EQ(args.at("name").as<std::string>(), "world");
}

TEST(CommandLineParserTest, ResultReadsBackThroughJsonReader)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer}, std::int64_t{1});
	parser.addOption("--check-stairs", {.type = Option::Type::Flag}, false);

	const char *argv[] = {"prog", "--seed", "42", "--check-stairs"};
	const spk::JSON::Value &args = parser.parse(4, argv);

	const spk::JSON::Reader reader(args, "<cli>");
	EXPECT_EQ(reader.optional<std::int64_t>("seed", 1), 42);
	EXPECT_TRUE(reader.optional<bool>("check-stairs", false));
	EXPECT_FALSE(reader.optional<bool>("map-only", false));
}

TEST(CommandLineParserTest, ArgumentsReturnsTheLastParseResult)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer}, std::int64_t{1});

	const char *argv[] = {"prog", "--seed", "7"};
	parser.parse(3, argv);

	EXPECT_EQ(parser.arguments().at("seed").as<std::int64_t>(), 7);
}

TEST(CommandLineParserTest, UnknownArgumentThrows)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer}, std::int64_t{1});

	const char *argv[] = {"prog", "--nope"};
	EXPECT_THROW(parser.parse(2, argv), Parser::Error);
}

TEST(CommandLineParserTest, MissingValueThrows)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer});

	const char *argv[] = {"prog", "--seed"};
	EXPECT_THROW(parser.parse(2, argv), Parser::Error);
}

TEST(CommandLineParserTest, MalformedIntegerValueThrows)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer});

	const char *argv[] = {"prog", "--seed", "12abc"};
	EXPECT_THROW(parser.parse(3, argv), Parser::Error);
}

TEST(CommandLineParserTest, DuplicateRegistrationThrows)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer});

	EXPECT_THROW(parser.addOption("--seed", {.type = Option::Type::Integer}), Parser::Error);
}

TEST(CommandLineParserTest, OptionNameWithoutDashThrows)
{
	Parser parser;

	EXPECT_THROW(parser.addOption("seed", {.type = Option::Type::Integer}), Parser::Error);
}

TEST(CommandLineParserTest, UsageListsRegisteredOptions)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer, .help = "world master seed"});
	parser.addOption("--verbose", "-v", {.type = Option::Type::Flag, .help = "chatty output"});

	const std::string usage = parser.usage();

	EXPECT_NE(usage.find("--seed"), std::string::npos);
	EXPECT_NE(usage.find("world master seed"), std::string::npos);
	EXPECT_NE(usage.find("-v"), std::string::npos);
}

TEST(CommandLineParserTest, ApplicationNameIsDerivedFromArgv0)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer});

	const char *argv[] = {"C:\\builds\\Playground.exe", "--seed", "1"};
	parser.parse(3, argv);

	EXPECT_EQ(parser.applicationName(), "Playground");
	EXPECT_NE(parser.usage().find("Usage: Playground"), std::string::npos);
}

TEST(CommandLineParserTest, ExplicitApplicationNameOverridesArgv0)
{
	Parser parser;
	parser.setApplicationName("worldgen");
	parser.addOption("--seed", {.type = Option::Type::Integer});

	const char *argv[] = {"C:\\builds\\Playground.exe", "--seed", "1"};
	parser.parse(3, argv);

	EXPECT_EQ(parser.applicationName(), "worldgen");
	EXPECT_NE(parser.usage().find("Usage: worldgen"), std::string::npos);
}

TEST(CommandLineParserTest, UsageSynopsisBracketsEveryOption)
{
	Parser parser;
	parser.setApplicationName("prog");
	parser.addOption("--seed", {.type = Option::Type::Integer, .help = "world master seed"});
	parser.addOption("--verbose", "-v", {.type = Option::Type::Flag, .help = "chatty output"});
	parser.addOption("--pos", {.type = Option::Type::Integer, .count = 2});

	const std::string usage = parser.usage();

	EXPECT_NE(usage.find("Usage: prog [--seed <int>] [--verbose] [--pos <int> <int>]"), std::string::npos);
	EXPECT_NE(usage.find("Options:"), std::string::npos);
	EXPECT_NE(usage.find("--verbose, -v"), std::string::npos);
}

TEST(CommandLineParserTest, UsageFallsBackToProgramWhenNameUnset)
{
	Parser parser;
	parser.addOption("--seed", {.type = Option::Type::Integer});

	EXPECT_NE(parser.usage().find("Usage: program"), std::string::npos);
}
