#pragma once

#include "structures/container/spk_uuid.hpp"

#include <optional>
#include <string_view>

namespace pg
{
	[[nodiscard]] inline std::optional<spk::UUID> uuidFromString(std::string_view p_value)
	{
		if (p_value.size() != 36 || p_value[8] != '-' || p_value[13] != '-' || p_value[18] != '-' || p_value[23] != '-')
		{
			return std::nullopt;
		}
		auto hex = [](char p_character) -> int {
			if (p_character >= '0' && p_character <= '9') return p_character - '0';
			if (p_character >= 'a' && p_character <= 'f') return p_character - 'a' + 10;
			if (p_character >= 'A' && p_character <= 'F') return p_character - 'A' + 10;
			return -1;
		};

		spk::UUID::Storage bytes{};
		std::size_t byteIndex = 0;
		for (std::size_t index = 0; index < p_value.size();)
		{
			if (p_value[index] == '-')
			{
				++index;
				continue;
			}
			const int high = hex(p_value[index++]);
			const int low = index < p_value.size() ? hex(p_value[index++]) : -1;
			if (high < 0 || low < 0 || byteIndex >= bytes.size())
			{
				return std::nullopt;
			}
			bytes[byteIndex++] = static_cast<std::uint8_t>((high << 4) | low);
		}
		if (byteIndex != bytes.size())
		{
			return std::nullopt;
		}
		return spk::UUID(bytes);
	}
}
