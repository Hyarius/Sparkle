#include "battle/battle_object.hpp"

#include <algorithm>
#include <cctype>

namespace
{
	[[nodiscard]] std::string normalize(std::string_view p_value)
	{
		const std::size_t first = p_value.find_first_not_of(" \t\r\n");
		if (first == std::string_view::npos)
		{
			return {};
		}
		const std::size_t last = p_value.find_last_not_of(" \t\r\n");
		std::string result(p_value.substr(first, last - first + 1));
		std::ranges::transform(result, result.begin(), [](unsigned char p_character) {
			return static_cast<char>(std::tolower(p_character));
		});
		return result;
	}
}

namespace pg
{
	bool BattleObject::hasTag(std::string_view p_tag) const
	{
		const std::string expected = normalize(p_tag);
		return !expected.empty() && std::ranges::any_of(tags, [&expected](const std::string &p_candidate) {
			return normalize(p_candidate) == expected;
		});
	}
}
