#pragma once

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/design_pattern/spk_singleton.hpp"

#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	// The single source of truth for every string a player reads. Definitions, widgets and the
	// HUD store translation keys, never sentences, and ask the translator for the text.
	//
	// It is a singleton because the alternative is threading a reference through every widget
	// that ever draws a word. Its lifetime is still explicitly owned - main() declares an
	// spk::Singleton<Translator>::Instanciator - so a load failure surfaces at startup rather
	// than at the first label that happens to be drawn.
	//
	// Language changes mid-run. Anything that caches a translated string must subscribe and
	// re-read on the callback, which is what makes a language switch a re-read rather than a
	// scene rebuild:
	//
	//     _contract = translator.subscribe([this](const std::string &) { _refreshLabels(); });
	//
	// The contract resigns when it is destroyed, so a widget that outlives nothing leaks no
	// dangling callback.
	class Translator : public spk::Singleton<Translator>
	{
		friend class spk::Singleton<Translator>;

	public:
		using LanguageChangedProvider = spk::ContractProvider<const std::string &>;
		using Contract = LanguageChangedProvider::Contract;

		// One language's key -> text map.
		using Entries = std::map<std::string, std::string, std::less<>>;

		~Translator() = default;

		// Loads every direct .json child of p_localeDirectory. The filename stem is the
		// language id ("en" -> locales/en.json), matching how every other registry names its
		// entries. Transactional: a failure leaves the previously loaded languages in place.
		//
		// The first language, in lexical order, becomes the active one when the translator had
		// none. Throws JsonError on a malformed file.
		void load(const std::filesystem::path &p_localeDirectory);

		// Lexically sorted, so a language menu built from this is stable.
		[[nodiscard]] const std::vector<std::string> &languages() const;
		[[nodiscard]] const std::string &language() const noexcept;

		// Throws std::invalid_argument on an unknown language. Notifies every subscriber, once,
		// after the switch has taken effect - so a callback that immediately calls text() reads
		// the new language, never the old one. Setting the active language again is a no-op and
		// notifies nobody.
		void setLanguage(const std::string &p_language);

		// A key with no text in the active language renders as the key itself: a missing
		// translation shows up as a visible gap in the UI instead of crashing a screen or
		// silently blanking a label.
		[[nodiscard]] std::string text(std::string_view p_key) const;

		// The same, with the arguments substituted into the translated text through std::format.
		// Placeholders are indexed - {0}, {1} - and never implicit, because a translation is
		// free to reorder them: the same two arguments read
		//
		//     "en": "{0} casts {1}!"
		//     "fr": "{1} : lancé par {0} !"
		//
		// Format specifications work as they do everywhere else ("{1:.1f}"), so a translator
		// controls padding and precision without the call site knowing.
		//
		// A text whose placeholders do not match the arguments given is left unsubstituted
		// rather than throwing: a wrong count in one line of one language must not take down
		// the screen drawing it.
		template <typename... TArguments>
			requires(sizeof...(TArguments) > 0)
		[[nodiscard]] std::string text(std::string_view p_key, const TArguments &...p_arguments) const
		{
			return _format(_entry(p_key), std::make_format_args(p_arguments...));
		}

		[[nodiscard]] bool contains(std::string_view p_key) const;

		[[nodiscard]] Contract subscribe(LanguageChangedProvider::Callback p_callback);

		[[nodiscard]] std::size_t keyCount() const noexcept;

	private:
		Translator() = default;

		// The active language's text for p_key, or a view of p_key itself when it has none.
		[[nodiscard]] std::string_view _entry(std::string_view p_key) const;
		[[nodiscard]] std::string _format(std::string_view p_text, std::format_args p_arguments) const;

		std::map<std::string, Entries, std::less<>> _languages;
		std::vector<std::string> _languageIds;
		std::string _language;
		LanguageChangedProvider _languageChanged;
	};

	// The active translator's text for p_key. Shorthand for the many call sites that only ever
	// read: it throws std::logic_error when no translator has been instanciated.
	template <typename... TArguments>
	[[nodiscard]] std::string tr(std::string_view p_key, const TArguments &...p_arguments)
	{
		return Translator::get().text(p_key, p_arguments...);
	}
}
