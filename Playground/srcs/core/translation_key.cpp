#include "core/translation_key.hpp"

#include "core/content_id.hpp"
#include "core/json.hpp"

namespace pg
{
	bool isTranslationKey(std::string_view p_value) noexcept
	{
		if (p_value.empty() || p_value.size() > MaxTranslationKeyLength)
		{
			return false;
		}

		// Every dot-separated segment is itself a content id, which also rejects an empty
		// segment - a leading, trailing or doubled dot.
		std::string_view remaining = p_value;
		while (true)
		{
			const std::size_t dot = remaining.find('.');
			if (dot == std::string_view::npos)
			{
				return isContentId(remaining);
			}
			if (!isContentId(remaining.substr(0, dot)))
			{
				return false;
			}
			remaining = remaining.substr(dot + 1);
		}
	}

	void requireTranslationKey(
		std::string_view p_value,
		const std::filesystem::path &p_file,
		std::string p_jsonPath,
		std::string_view p_kind)
	{
		if (isTranslationKey(p_value))
		{
			return;
		}

		throw JsonError(
			p_file,
			std::move(p_jsonPath),
			"invalid " + std::string(p_kind) + " '" + std::string(p_value) + "': expected 1 to " +
				std::to_string(MaxTranslationKeyLength) +
				" bytes matching [a-z0-9]+(-[a-z0-9]+)*(.[a-z0-9]+(-[a-z0-9]+)*)*");
	}
}
