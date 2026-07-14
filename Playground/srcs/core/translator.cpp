#include "core/translator.hpp"

#include "core/content_id.hpp"
#include "core/definition_fields.hpp"
#include "core/json.hpp"
#include "core/translation_key.hpp"

#include <algorithm>
#include <stdexcept>
#include <system_error>

namespace pg
{
	namespace
	{
		constexpr int LocaleSchemaVersion = 1;

		// The entries live under their own key rather than at the root: a flat root map could
		// never carry a version, because a translation key spelled "version" would collide
		// with it.
		[[nodiscard]] Translator::Entries parseLocale(JsonReader &p_reader)
		{
			p_reader.forbidUnknown({"version", "entries"});
			requireVersion(p_reader, LocaleSchemaVersion);

			const JsonReader entriesReader = p_reader.child("entries");

			Translator::Entries result;
			for (const auto &[key, unused] : entriesReader.value().asObject())
			{
				(void)unused;
				requireTranslationKey(key, entriesReader.file(), entriesReader.pathFor(key), "translation key");
				// A blank translation renders as an empty label, which is harder to spot than a
				// visible key, so it is rejected like any other malformed value.
				result.emplace(key, requireBoundedText(entriesReader, key, MaxTranslationTextBytes));
			}
			return result;
		}
	}

	void Translator::load(const std::filesystem::path &p_localeDirectory)
	{
		std::error_code error;
		if (!std::filesystem::is_directory(p_localeDirectory, error) || error)
		{
			throw JsonError(p_localeDirectory, "$", "locale directory does not exist");
		}

		// Built in locals and published at the end, so a broken locale file leaves the running
		// language untouched instead of half-replacing it.
		std::map<std::string, Entries, std::less<>> loaded;
		std::vector<std::filesystem::path> files;
		for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(p_localeDirectory))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				files.push_back(entry.path());
			}
		}
		std::ranges::sort(files, {}, [](const std::filesystem::path &p_path) {
			return p_path.filename().generic_string();
		});

		std::vector<std::string> languageIds;
		for (const std::filesystem::path &file : files)
		{
			const std::string language = file.stem().string();
			requireContentId(language, file, "$", "language id");

			const spk::JSON::Value json = JsonLoader::parseFile(file);
			JsonReader reader(json, file);
			loaded.emplace(language, parseLocale(reader));
			languageIds.push_back(language);
		}

		if (loaded.empty())
		{
			throw JsonError(p_localeDirectory, "$", "no locale file found");
		}

		_languages = std::move(loaded);
		_languageIds = std::move(languageIds);
		// A reload that drops the active language falls back to the first one rather than
		// leaving every lookup pointing at a language that no longer exists.
		if (_language.empty() || !_languages.contains(_language))
		{
			_language = _languageIds.front();
		}
	}

	const std::vector<std::string> &Translator::languages() const
	{
		return _languageIds;
	}

	const std::string &Translator::language() const noexcept
	{
		return _language;
	}

	void Translator::setLanguage(const std::string &p_language)
	{
		if (!_languages.contains(p_language))
		{
			throw std::invalid_argument("unknown language '" + p_language + "'");
		}
		if (_language == p_language)
		{
			return;
		}

		_language = p_language;
		_languageChanged.trigger(_language);
	}

	std::string_view Translator::_entry(std::string_view p_key) const
	{
		const auto language = _languages.find(_language);
		if (language != _languages.end())
		{
			const auto entry = language->second.find(p_key);
			if (entry != language->second.end())
			{
				return entry->second;
			}
		}
		// A view of the caller's key, which outlives the call.
		return p_key;
	}

	std::string Translator::_format(std::string_view p_text, std::format_args p_arguments) const
	{
		try
		{
			return std::vformat(p_text, p_arguments);
		} catch (const std::format_error &)
		{
			// The translated line and the call site disagree on the placeholders. Show the
			// unsubstituted text: one wrong line is a bug to fix, not a reason to take down the
			// screen that draws it.
			return std::string(p_text);
		}
	}

	std::string Translator::text(std::string_view p_key) const
	{
		// No arguments, so the text is never handed to the formatter: a stray brace in an
		// ordinary sentence is not an error.
		return std::string(_entry(p_key));
	}

	bool Translator::contains(std::string_view p_key) const
	{
		const auto language = _languages.find(_language);
		return language != _languages.end() && language->second.contains(p_key);
	}

	Translator::Contract Translator::subscribe(LanguageChangedProvider::Callback p_callback)
	{
		return _languageChanged.subscribe(std::move(p_callback));
	}

	std::size_t Translator::keyCount() const noexcept
	{
		const auto language = _languages.find(_language);
		return language == _languages.end() ? 0 : language->second.size();
	}
}
