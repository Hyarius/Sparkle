#include "core/content_id.hpp"

#include "core/json.hpp"

namespace pg
{
	namespace
	{
		[[nodiscard]] bool isContentIdCharacter(char p_character) noexcept
		{
			return (p_character >= 'a' && p_character <= 'z') || (p_character >= '0' && p_character <= '9') ||
				   p_character == '-';
		}
	}

	bool isContentId(std::string_view p_value) noexcept
	{
		if (p_value.empty() || p_value.size() > MaxContentIdLength)
		{
			return false;
		}
		if (p_value.front() == '-' || p_value.back() == '-')
		{
			return false;
		}

		bool previousWasHyphen = false;
		for (const char character : p_value)
		{
			if (!isContentIdCharacter(character))
			{
				return false;
			}
			if (character == '-' && previousWasHyphen)
			{
				return false;
			}
			previousWasHyphen = (character == '-');
		}
		return true;
	}

	void requireContentId(
		std::string_view p_value,
		const std::filesystem::path &p_file,
		std::string p_jsonPath,
		std::string_view p_kind)
	{
		if (isContentId(p_value))
		{
			return;
		}

		throw JsonError(
			p_file,
			std::move(p_jsonPath),
			"invalid " + std::string(p_kind) + " '" + std::string(p_value) +
				"': expected 1 to " + std::to_string(MaxContentIdLength) +
				" bytes matching [a-z0-9]+(-[a-z0-9]+)*");
	}
}
