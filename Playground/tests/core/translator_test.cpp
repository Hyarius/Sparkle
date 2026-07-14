#include <gtest/gtest.h>

#include "core/json.hpp"
#include "core/paths.hpp"
#include "core/translation_key.hpp"
#include "core/translator.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	// A scratch locale directory, so a case authors exactly the languages it is about.
	class LocaleDirectory
	{
	private:
		std::filesystem::path _path;

	public:
		explicit LocaleDirectory(std::string_view p_name) :
			_path(std::filesystem::temp_directory_path() / "sparkle-playground-locales" / p_name)
		{
			std::filesystem::remove_all(_path);
			std::filesystem::create_directories(_path);
		}

		~LocaleDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		LocaleDirectory(const LocaleDirectory &) = delete;
		LocaleDirectory &operator=(const LocaleDirectory &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}

		void write(std::string_view p_language, std::string_view p_content) const
		{
			std::ofstream stream(_path / (std::string(p_language) + ".json"), std::ios::binary | std::ios::trunc);
			stream << p_content;
		}
	};

	// Each case owns the translator for its own scope.
	class TranslatorTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			pg::Translator::release();
			pg::Translator::instanciate();
		}

		void TearDown() override
		{
			pg::Translator::release();
		}

		[[nodiscard]] static pg::Translator &translator()
		{
			return pg::Translator::get();
		}
	};
}

TEST(TranslationKeyTest, AcceptsDotSeparatedContentIdSegments)
{
	EXPECT_TRUE(pg::isTranslationKey("ability.training-strike.name"));
	EXPECT_TRUE(pg::isTranslationKey("name")) << "a single segment is a key";
	EXPECT_TRUE(pg::isTranslationKey("ui.hud.slot-1"));

	EXPECT_FALSE(pg::isTranslationKey("")) << "empty";
	EXPECT_FALSE(pg::isTranslationKey("Ability.name")) << "upper case would need locale-dependent folding";
	EXPECT_FALSE(pg::isTranslationKey("ability..name")) << "an empty segment";
	EXPECT_FALSE(pg::isTranslationKey(".ability")) << "a leading dot";
	EXPECT_FALSE(pg::isTranslationKey("ability.")) << "a trailing dot";
	EXPECT_FALSE(pg::isTranslationKey("ability.-name")) << "a segment starting with a hyphen";
	EXPECT_FALSE(pg::isTranslationKey("ability name")) << "a space";
	EXPECT_FALSE(pg::isTranslationKey(std::string(129, 'a'))) << "longer than the maximum";

	// A sentence is exactly what a definition may no longer carry.
	EXPECT_FALSE(pg::isTranslationKey("Training Strike"));
}

TEST_F(TranslatorTest, LoadsTheShippedLocalesAndActivatesTheFirstLanguage)
{
	translator().load(pg::resourceRoot() / "data" / "locales");

	EXPECT_EQ(translator().languages(), (std::vector<std::string>{"en", "fr"})) << "lexically sorted";
	EXPECT_EQ(translator().language(), "en") << "the first language, in lexical order";
	EXPECT_GT(translator().keyCount(), 0U);

	EXPECT_EQ(translator().text("ability.training-strike.name"), "Training Strike");
	EXPECT_EQ(translator().text("status.training-guarded.name"), "Guarded");
}

TEST_F(TranslatorTest, SwitchesLanguageAndNotifiesEverySubscriberAfterTheSwitchTookEffect)
{
	translator().load(pg::resourceRoot() / "data" / "locales");

	int notifications = 0;
	std::string seenLanguage;
	std::string textReadFromTheCallback;
	const pg::Translator::Contract contract =
		translator().subscribe([&](const std::string &p_language) {
			++notifications;
			seenLanguage = p_language;
			// A callback that immediately re-reads must see the new language, never the old one.
			textReadFromTheCallback = pg::Translator::get().text("ability.training-strike.name");
		});

	translator().setLanguage("fr");

	EXPECT_EQ(notifications, 1);
	EXPECT_EQ(seenLanguage, "fr");
	EXPECT_EQ(textReadFromTheCallback, "Frappe d'entraînement");
	EXPECT_EQ(translator().text("ability.training-strike.name"), "Frappe d'entraînement");
	EXPECT_EQ(translator().language(), "fr");

	// Setting the active language again changes nothing and notifies nobody.
	translator().setLanguage("fr");
	EXPECT_EQ(notifications, 1);

	EXPECT_THROW(translator().setLanguage("de"), std::invalid_argument);
	EXPECT_EQ(translator().language(), "fr") << "a rejected switch leaves the active language alone";
	EXPECT_EQ(notifications, 1);
}

TEST_F(TranslatorTest, AResignedSubscriberIsNoLongerNotified)
{
	translator().load(pg::resourceRoot() / "data" / "locales");

	int notifications = 0;
	{
		const pg::Translator::Contract contract =
			translator().subscribe([&notifications](const std::string &) { ++notifications; });
		translator().setLanguage("fr");
		EXPECT_EQ(notifications, 1);
	}

	// The contract resigned when it went out of scope, so a widget that is gone leaves no
	// dangling callback behind.
	translator().setLanguage("en");
	EXPECT_EQ(notifications, 1);
}

TEST_F(TranslatorTest, AMissingKeyRendersAsItself)
{
	translator().load(pg::resourceRoot() / "data" / "locales");

	EXPECT_FALSE(translator().contains("ability.ghost.name"));
	EXPECT_EQ(translator().text("ability.ghost.name"), "ability.ghost.name")
		<< "a missing translation is a visible gap, not a crash and not a blank label";
	EXPECT_TRUE(translator().contains("ability.training-strike.name"));
}

TEST_F(TranslatorTest, SubstitutesIndexedArgumentsInTheOrderTheTranslationChooses)
{
	translator().load(pg::resourceRoot() / "data" / "locales");

	EXPECT_EQ(translator().text("battle.log.damage-dealt", "Kabu", 12), "Kabu takes 12 damage.");
	EXPECT_EQ(translator().text("battle.log.ability-cast", "Kabu", "Training Strike"), "Kabu casts Training Strike!");

	// The same two arguments, reordered by the translation itself: this is why placeholders are
	// indexed and never implicit.
	translator().setLanguage("fr");
	EXPECT_EQ(
		translator().text("battle.log.ability-cast", "Kabu", "Frappe d'entraînement"),
		"Frappe d'entraînement : lancé par Kabu !");
	EXPECT_EQ(translator().text("battle.log.damage-dealt", "Kabu", 12), "Kabu subit 12 points de dégâts.");
}

TEST_F(TranslatorTest, LeavesATextWhosePlaceholdersDoNotMatchTheArgumentsUnsubstituted)
{
	const LocaleDirectory locales("bad-format");
	locales.write("en", R"({
		"version": 1,
		"entries": {
			"greeting": "Hello {0}, you have {1} messages.",
			"broken": "Hello {7}!",
			"literal-brace": "Reduces cost by 50% {not a placeholder}"
		}
	})");
	translator().load(locales.path());

	EXPECT_EQ(translator().text("greeting", "Kabu", 3), "Hello Kabu, you have 3 messages.");

	// One wrong line in one language must not take down the screen drawing it.
	EXPECT_EQ(translator().text("broken", "Kabu"), "Hello {7}!");

	// With no arguments the text never reaches the formatter, so a stray brace in an ordinary
	// sentence is not an error.
	EXPECT_EQ(translator().text("literal-brace"), "Reduces cost by 50% {not a placeholder}");
}

TEST_F(TranslatorTest, RejectsMalformedLocaleFiles)
{
	const LocaleDirectory badVersion("bad-version");
	badVersion.write("en", R"({"version": 2, "entries": {"greeting": "Hello"}})");
	EXPECT_THROW(translator().load(badVersion.path()), pg::JsonError) << "an unsupported version";

	const LocaleDirectory unknownField("unknown-field");
	unknownField.write("en", R"({"version": 1, "entries": {"greeting": "Hello"}, "fallback": "fr"})");
	EXPECT_THROW(translator().load(unknownField.path()), pg::JsonError) << "an unknown root field";

	const LocaleDirectory badKey("bad-key");
	badKey.write("en", R"({"version": 1, "entries": {"Greeting": "Hello"}})");
	EXPECT_THROW(translator().load(badKey.path()), pg::JsonError) << "a key outside the key grammar";

	const LocaleDirectory blank("blank-text");
	blank.write("en", R"({"version": 1, "entries": {"greeting": "   "}})");
	EXPECT_THROW(translator().load(blank.path()), pg::JsonError) << "a blank translation renders as an empty label";

	const LocaleDirectory notAString("not-a-string");
	notAString.write("en", R"({"version": 1, "entries": {"greeting": 12}})");
	EXPECT_THROW(translator().load(notAString.path()), pg::JsonError);

	const LocaleDirectory badLanguage("bad-language");
	badLanguage.write("EN", R"({"version": 1, "entries": {"greeting": "Hello"}})");
	EXPECT_THROW(translator().load(badLanguage.path()), pg::JsonError) << "the language id is the filename stem";

	const LocaleDirectory empty("no-locale");
	EXPECT_THROW(translator().load(empty.path()), pg::JsonError) << "a locale directory with no language in it";

	EXPECT_THROW(translator().load(empty.path() / "missing"), pg::JsonError) << "a directory that does not exist";
}

TEST_F(TranslatorTest, AFailedReloadLeavesTheLoadedLanguagesAndTheActiveOneUntouched)
{
	translator().load(pg::resourceRoot() / "data" / "locales");
	translator().setLanguage("fr");

	const LocaleDirectory broken("broken-reload");
	broken.write("en", R"({"version": 1, "entries": {"greeting": "Hello"}})");
	broken.write("zz", "{ this is not json");

	EXPECT_THROW(translator().load(broken.path()), pg::JsonError);

	EXPECT_EQ(translator().languages(), (std::vector<std::string>{"en", "fr"}));
	EXPECT_EQ(translator().language(), "fr");
	EXPECT_EQ(translator().text("ability.training-strike.name"), "Frappe d'entraînement");
}

TEST_F(TranslatorTest, AReloadThatDropsTheActiveLanguageFallsBackToTheFirstOne)
{
	translator().load(pg::resourceRoot() / "data" / "locales");
	translator().setLanguage("fr");

	const LocaleDirectory englishOnly("english-only");
	englishOnly.write("en", R"({"version": 1, "entries": {"greeting": "Hello"}})");
	translator().load(englishOnly.path());

	EXPECT_EQ(translator().language(), "en") << "every lookup would otherwise point at a language that is gone";
	EXPECT_EQ(translator().text("greeting"), "Hello");
}
