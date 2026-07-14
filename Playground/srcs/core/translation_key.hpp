#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace pg
{
	// The grammar of an authored translation key: dot-separated content-id segments.
	//
	//     [a-z0-9]+(?:-[a-z0-9]+)*(?:\.[a-z0-9]+(?:-[a-z0-9]+)*)*      1 to 128 ASCII bytes
	//
	// The dots are a naming convention, not a tree: nothing nests, and the whole key is one
	// lookup. The convention is <domain>.<id>.<field>, so a definition's text keys read
	//
	//     ability.training-strike.name
	//     ability.training-strike.description
	//
	// Lower case only, matching the content-id rule, so no locale-dependent case folding ever
	// decides whether two keys are the same.
	inline constexpr std::size_t MaxTranslationKeyLength = 128;

	[[nodiscard]] bool isTranslationKey(std::string_view p_value) noexcept;

	// Throws JsonError naming the file, the JSON path, the offending value and the grammar.
	void requireTranslationKey(
		std::string_view p_value,
		const std::filesystem::path &p_file,
		std::string p_jsonPath,
		std::string_view p_kind);
}
