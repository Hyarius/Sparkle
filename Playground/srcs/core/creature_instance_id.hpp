#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pg
{
	// The identity a creature keeps across saves, parties, boxes and battles.
	//
	//     creature-000000000000002a
	//
	// The prefix, exactly sixteen lower-case hexadecimal digits, zero padded, encoding a
	// non-zero uint64 serial. The text is the persisted form and parses back to the same
	// serial, which no std::hash or opaque UUID blob would guarantee.
	//
	// Serials come from PlayerData::nextCreatureSerial (step 04/18) and are therefore
	// deterministic: a recruit is never identified by std::random_device.
	class CreatureInstanceId
	{
	public:
		static constexpr std::string_view Prefix = "creature-";
		static constexpr std::size_t SerialDigits = 16;
		static constexpr std::size_t TextLength = 25;

		// Default construction is the invalid id, matching serial 0.
		constexpr CreatureInstanceId() noexcept = default;

		// Throws std::invalid_argument when p_serial is zero.
		[[nodiscard]] static CreatureInstanceId fromSerial(std::uint64_t p_serial);

		// Throws std::invalid_argument on anything but the canonical text: upper case,
		// missing padding, a sign, whitespace, a wrong length, a non-hex digit, or zero.
		[[nodiscard]] static CreatureInstanceId parse(std::string_view p_text);

		[[nodiscard]] std::uint64_t serial() const noexcept;

		// Empty for the invalid id. Points into the id itself, so it lives exactly as long.
		[[nodiscard]] std::string_view string() const noexcept;

		[[nodiscard]] bool valid() const noexcept;

		[[nodiscard]] bool operator==(const CreatureInstanceId &p_other) const noexcept = default;
		[[nodiscard]] auto operator<=>(const CreatureInstanceId &p_other) const noexcept = default;

	private:
		std::uint64_t _serial = 0;
		// Storing the text inline keeps the id trivially copyable and lets string() hand out
		// a view without allocating. NUL filled, so the invalid id views an empty string.
		std::array<char, TextLength + 1> _text{};
	};
}
