#pragma once

#include <cstdint>
#include <string_view>

namespace pg::deterministic
{
	inline constexpr std::uint64_t FnvOffset = 1469598103934665603ULL;
	inline constexpr std::uint64_t FnvPrime = 1099511628211ULL;

	inline void mix(std::uint64_t &p_hash, std::string_view p_value) noexcept
	{
		for (const char character : p_value)
		{
			p_hash ^= static_cast<std::uint8_t>(character);
			p_hash *= FnvPrime;
		}
	}

	inline void mix(std::uint64_t &p_hash, int p_value) noexcept
	{
		p_hash ^= static_cast<std::uint32_t>(p_value);
		p_hash *= FnvPrime;
	}

	[[nodiscard]] inline std::uint64_t fnv1a(std::string_view p_value) noexcept
	{
		std::uint64_t hash = FnvOffset;
		mix(hash, p_value);
		return hash;
	}

	[[nodiscard]] inline std::uint64_t avalanche(std::uint64_t p_value) noexcept
	{
		p_value ^= p_value >> 30U;
		p_value *= 0xbf58476d1ce4e5b9ULL;
		p_value ^= p_value >> 27U;
		p_value *= 0x94d049bb133111ebULL;
		p_value ^= p_value >> 31U;
		return p_value;
	}

	[[nodiscard]] inline double unitInterval(std::uint64_t p_hash) noexcept
	{
		return static_cast<double>(p_hash >> 11U) * (1.0 / 9007199254740992.0);
	}
}
