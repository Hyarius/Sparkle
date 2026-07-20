#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

namespace spk
{
	class UUID
	{
	public:
		static constexpr std::size_t Size = 16;
		using Storage = std::array<std::uint8_t, Size>;

	private:
		Storage _bytes{};

	public:
		constexpr UUID() noexcept = default;
		explicit constexpr UUID(Storage p_bytes) noexcept :
			_bytes(p_bytes)
		{
		}

		[[nodiscard]] static UUID generate();
		[[nodiscard]] static constexpr UUID null() noexcept
		{
			return UUID();
		}
		[[nodiscard]] static UUID fromString(std::string_view p_string);
		[[nodiscard]] static std::optional<UUID> tryParse(std::string_view p_string) noexcept;

		[[nodiscard]] constexpr const Storage &bytes() const noexcept
		{
			return _bytes;
		}
		[[nodiscard]] constexpr bool isNull() const noexcept
		{
			for (const std::uint8_t byte : _bytes)
			{
				if (byte != 0u)
				{
					return false;
				}
			}
			return true;
		}
		[[nodiscard]] constexpr std::uint8_t version() const noexcept
		{
			return static_cast<std::uint8_t>(_bytes[6] >> 4u);
		}
		[[nodiscard]] constexpr bool hasRFCVariant() const noexcept
		{
			return (_bytes[8] & 0xC0u) == 0x80u;
		}
		[[nodiscard]] std::string toString() const;

		[[nodiscard]] constexpr auto operator<=>(const UUID &) const noexcept = default;
	};

	std::ostream &operator<<(std::ostream &p_stream, const UUID &p_uuid);
}

namespace std
{
	template <>
	struct hash<spk::UUID>
	{
		[[nodiscard]] std::size_t operator()(const spk::UUID &p_uuid) const noexcept;
	};
}
