#include "structures/container/spk_uuid.hpp"

#include <iomanip>
#include <optional>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace spk
{
	UUID UUID::generate()
	{
		static thread_local std::mt19937_64 generator(std::random_device{}());
		std::uniform_int_distribution<std::uint32_t> distribution(0u, 255u);

		Storage bytes{};
		for (std::uint8_t &byte : bytes)
		{
			byte = static_cast<std::uint8_t>(distribution(generator));
		}

		bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0Fu) | 0x40u);
		bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3Fu) | 0x80u);

		return UUID(bytes);
	}

	std::optional<UUID> UUID::tryParse(std::string_view p_string) noexcept
	{
		if (p_string.size() != 36 || p_string[8] != '-' || p_string[13] != '-' || p_string[18] != '-' || p_string[23] != '-')
		{
			return std::nullopt;
		}
		auto hexValue = [](const char p_character) constexpr -> int {
			if (p_character >= '0' && p_character <= '9')
			{
				return p_character - '0';
			}
			if (p_character >= 'a' && p_character <= 'f')
			{
				return 10 + p_character - 'a';
			}
			if (p_character >= 'A' && p_character <= 'F')
			{
				return 10 + p_character - 'A';
			}
			return -1;
		};
		Storage bytes{};
		std::size_t output = 0;
		for (std::size_t input = 0; input < p_string.size();)
		{
			if (p_string[input] == '-')
			{
				++input;
				continue;
			}
			const int high = hexValue(p_string[input++]);
			const int low = hexValue(p_string[input++]);
			if (high < 0 || low < 0)
			{
				return std::nullopt;
			}
			bytes[output++] = static_cast<std::uint8_t>((high << 4) | low);
		}
		return UUID(bytes);
	}

	UUID UUID::fromString(std::string_view p_string)
	{
		if (const std::optional<UUID> parsed = tryParse(p_string); parsed.has_value())
		{
			return *parsed;
		}
		throw std::invalid_argument("spk::UUID: invalid UUID string");
	}

	std::string UUID::toString() const
	{
		std::ostringstream stream;
		stream << std::hex << std::setfill('0');

		for (std::size_t i = 0; i < _bytes.size(); ++i)
		{
			if (i == 4u || i == 6u || i == 8u || i == 10u)
			{
				stream << '-';
			}

			stream << std::setw(2) << static_cast<unsigned int>(_bytes[i]);
		}

		return stream.str();
	}

	std::ostream &operator<<(std::ostream &p_stream, const UUID &p_uuid)
	{
		p_stream << p_uuid.toString();
		return p_stream;
	}
}

namespace std
{
	std::size_t hash<spk::UUID>::operator()(const spk::UUID &p_uuid) const noexcept
	{
		std::uint64_t result = 14695981039346656037ull;

		for (std::uint8_t byte : p_uuid.bytes())
		{
			result ^= static_cast<std::uint64_t>(byte);
			result *= 1099511628211ull;
		}

		if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t))
		{
			return static_cast<std::size_t>(result);
		}
		else
		{
			return static_cast<std::size_t>(result ^ (result >> 32u));
		}
	}
}
