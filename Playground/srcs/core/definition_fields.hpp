#pragma once

#include "core/json.hpp"
#include "structures/math/spk_vector2.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace pg
{
	// The authored metadata every combat definition shares, with the exact bounds the combat
	// schema locks. Bounds are checked, never repaired: a value is not trimmed, an id is not
	// case-folded, and an out-of-range number is an error rather than a clamp.
	//
	// A definition never carries a sentence: it carries the translation key of one. The text
	// itself lives in resources/data/locales and is bounded there.
	inline constexpr std::size_t MaxTranslationTextBytes = 512;
	inline constexpr int MaxIconCoordinate = 4095;

	// Where a nested value was authored. Cross-definition references are validated in a second
	// phase, long after the reader that parsed them is gone, and their diagnostics must still
	// name the exact file and JSON path. A Feat Board's form references are validated later
	// still - in step 04, once a species selects the board - from this same record.
	struct DefinitionSource
	{
		std::filesystem::path file;
		std::string jsonPath;
	};

	[[nodiscard]] std::int64_t requireIntegerInRange(
		const JsonReader &p_reader,
		const std::string &p_key,
		std::int64_t p_minimum,
		std::int64_t p_maximum);

	// A required version field that must equal p_supported exactly. An older file fails loudly
	// instead of half-loading against a schema it was not written for.
	void requireVersion(const JsonReader &p_reader, int p_supported);

	// 1 to p_maximumBytes UTF-8 bytes, and never blank.
	[[nodiscard]] std::string requireBoundedText(
		const JsonReader &p_reader,
		const std::string &p_key,
		std::size_t p_maximumBytes);

	// A translation key the Translator resolves at display time. It is not checked against the
	// loaded languages: a key with no text renders as itself, so a missing translation is a
	// visible gap rather than a startup failure, and content can be authored before it is
	// translated.
	[[nodiscard]] std::string requireTranslationKeyField(
		const JsonReader &p_reader,
		const std::string &p_key,
		std::string_view p_kind);

	[[nodiscard]] std::string requireDisplayNameKey(const JsonReader &p_reader);
	[[nodiscard]] std::string requireDescriptionKey(const JsonReader &p_reader);

	// The atlas cell, authored as [x, y], both coordinates in [0, MaxIconCoordinate].
	[[nodiscard]] spk::Vector2Int requireIcon(const JsonReader &p_reader);

	// Content-id tags in authored order, rejected on a duplicate. p_allowEmpty separates a
	// definition's own tag list, which may be empty, from a cleanse/removal filter, which
	// selects nothing when empty and is therefore always a bug.
	[[nodiscard]] std::vector<std::string> requireTags(
		const JsonReader &p_reader,
		const std::string &p_key,
		bool p_allowEmpty);
}
