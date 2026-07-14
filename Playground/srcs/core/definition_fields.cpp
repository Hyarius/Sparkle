#include "core/definition_fields.hpp"

#include "core/content_id.hpp"
#include "core/translation_key.hpp"

#include <set>

namespace pg
{
	namespace
	{
		[[nodiscard]] bool isBlank(const std::string &p_text) noexcept
		{
			for (const char character : p_text)
			{
				if (character != ' ' && character != '\t' && character != '\n' && character != '\r')
				{
					return false;
				}
			}
			return true;
		}
	}

	std::int64_t requireIntegerInRange(
		const JsonReader &p_reader,
		const std::string &p_key,
		std::int64_t p_minimum,
		std::int64_t p_maximum)
	{
		const std::int64_t value = p_reader.require<std::int64_t>(p_key);
		if (value < p_minimum || value > p_maximum)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor(p_key),
				"expected an integer in [" + std::to_string(p_minimum) + ", " + std::to_string(p_maximum) + "], got " +
					std::to_string(value));
		}
		return value;
	}

	void requireVersion(const JsonReader &p_reader, int p_supported)
	{
		if (p_reader.require<int>("version") != p_supported)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version (expected " + std::to_string(p_supported) + ")");
		}
	}

	std::string requireBoundedText(const JsonReader &p_reader, const std::string &p_key, std::size_t p_maximumBytes)
	{
		std::string value = p_reader.require<std::string>(p_key);
		if (value.empty() || value.size() > p_maximumBytes)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor(p_key),
				"expected 1 to " + std::to_string(p_maximumBytes) + " UTF-8 bytes, got " +
					std::to_string(value.size()));
		}
		if (isBlank(value))
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected text, got whitespace only");
		}
		return value;
	}

	std::string requireTranslationKeyField(
		const JsonReader &p_reader,
		const std::string &p_key,
		std::string_view p_kind)
	{
		std::string value = p_reader.require<std::string>(p_key);
		requireTranslationKey(value, p_reader.file(), p_reader.pathFor(p_key), p_kind);
		return value;
	}

	std::string requireDisplayNameKey(const JsonReader &p_reader)
	{
		return requireTranslationKeyField(p_reader, "displayNameKey", "display name key");
	}

	std::string requireDescriptionKey(const JsonReader &p_reader)
	{
		return requireTranslationKeyField(p_reader, "descriptionKey", "description key");
	}

	spk::Vector2Int requireIcon(const JsonReader &p_reader)
	{
		const spk::Vector2Int icon = p_reader.require<spk::Vector2Int>("icon");
		if (icon.x < 0 || icon.x > MaxIconCoordinate || icon.y < 0 || icon.y > MaxIconCoordinate)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("icon"),
				"icon coordinates are integers in [0, " + std::to_string(MaxIconCoordinate) + "], got " +
					icon.toString());
		}
		return icon;
	}

	std::vector<std::string> requireTags(const JsonReader &p_reader, const std::string &p_key, bool p_allowEmpty)
	{
		std::vector<std::string> tags = p_reader.require<std::vector<std::string>>(p_key);
		if (tags.empty() && !p_allowEmpty)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "expected at least one tag");
		}

		std::set<std::string> seen;
		for (const std::string &tag : tags)
		{
			requireContentId(tag, p_reader.file(), p_reader.pathFor(p_key), "tag");
			if (!seen.insert(tag).second)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor(p_key), "duplicate tag '" + tag + "'");
			}
		}
		return tags;
	}
}
