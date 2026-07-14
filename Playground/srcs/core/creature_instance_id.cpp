#include "core/creature_instance_id.hpp"

#include <stdexcept>
#include <string>

namespace pg
{
	namespace
	{
		constexpr std::string_view HexDigits = "0123456789abcdef";

		// Hand rolled instead of std::stoull or a stream: those accept a leading sign, skip
		// whitespace, and fold case depending on the locale. None of that may decide whether
		// a persisted creature id is valid.
		[[nodiscard]] bool tryHexValue(char p_character, std::uint64_t &p_value) noexcept
		{
			if (p_character >= '0' && p_character <= '9')
			{
				p_value = static_cast<std::uint64_t>(p_character - '0');
				return true;
			}
			if (p_character >= 'a' && p_character <= 'f')
			{
				p_value = static_cast<std::uint64_t>(p_character - 'a') + 10U;
				return true;
			}
			return false;
		}
	}

	CreatureInstanceId CreatureInstanceId::fromSerial(std::uint64_t p_serial)
	{
		if (p_serial == 0U)
		{
			throw std::invalid_argument("a creature serial of zero is the invalid sentinel");
		}

		CreatureInstanceId result;
		result._serial = p_serial;

		std::size_t index = 0;
		for (const char character : Prefix)
		{
			result._text[index] = character;
			++index;
		}
		// Exactly sixteen digits, most significant first, so the text sorts like the serial.
		for (std::size_t digit = 0; digit < SerialDigits; ++digit)
		{
			const unsigned int shift = static_cast<unsigned int>((SerialDigits - 1U - digit) * 4U);
			const std::uint64_t nibble = (p_serial >> shift) & 0xFU;
			result._text[index] = HexDigits[static_cast<std::size_t>(nibble)];
			++index;
		}
		result._text[index] = '\0';
		return result;
	}

	CreatureInstanceId CreatureInstanceId::parse(std::string_view p_text)
	{
		const auto reject = [&p_text](const char *p_reason) {
			throw std::invalid_argument(
				"invalid creature instance id '" + std::string(p_text) + "': " + p_reason);
		};

		if (p_text.size() != TextLength)
		{
			reject("expected exactly 25 characters, 'creature-' followed by 16 hexadecimal digits");
		}
		if (p_text.substr(0, Prefix.size()) != Prefix)
		{
			reject("expected the 'creature-' prefix");
		}

		std::uint64_t serial = 0;
		for (const char character : p_text.substr(Prefix.size()))
		{
			std::uint64_t digit = 0;
			if (!tryHexValue(character, digit))
			{
				reject("expected 16 lower-case hexadecimal digits");
			}
			// Exactly sixteen digits fill a uint64 to the brim, so this shift never overflows
			// once the length has been checked.
			serial = (serial << 4U) | digit;
		}

		if (serial == 0U)
		{
			reject("a creature serial of zero is the invalid sentinel");
		}

		return fromSerial(serial);
	}

	std::uint64_t CreatureInstanceId::serial() const noexcept
	{
		return _serial;
	}

	std::string_view CreatureInstanceId::string() const noexcept
	{
		return std::string_view(_text.data());
	}

	bool CreatureInstanceId::valid() const noexcept
	{
		return _serial != 0U;
	}
}
