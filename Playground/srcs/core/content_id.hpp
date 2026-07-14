#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace pg
{
	// One grammar for every authored identifier introduced by the battle content: definition
	// filename stems, Feat node ids, condition ids, effect ids, AI rule ids, team ids and
	// trigger ids.
	//
	//     [a-z0-9]+(?:-[a-z0-9]+)*      1 to 64 ASCII bytes
	//
	// Lower case only, so no locale-dependent case folding ever decides whether two ids are
	// the same. Local ids are scoped by their owning definition: "root" may appear in two
	// Feat Boards, but not twice in one.
	inline constexpr std::size_t MaxContentIdLength = 64;

	[[nodiscard]] bool isContentId(std::string_view p_value) noexcept;

	// Throws JsonError naming the file, the JSON path, the offending value and the grammar.
	// p_kind names the domain ("feat node id", "ability id") so the message is actionable.
	void requireContentId(
		std::string_view p_value,
		const std::filesystem::path &p_file,
		std::string p_jsonPath,
		std::string_view p_kind);
}
