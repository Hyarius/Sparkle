#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace spk::deterministic
{
	inline constexpr std::uint64_t FnvOffset = 1469598103934665603ULL;
	inline constexpr std::uint64_t FnvPrime = 1099511628211ULL;

	// StableHasher64 deliberately defines its mixing operations instead of relying on
	// std::hash, whose output is not a cross-platform persistence contract.
	class StableHasher64
	{
	private:
		std::uint64_t _value = FnvOffset;

	public:
		StableHasher64() = default;
		explicit StableHasher64(std::uint64_t p_initialValue) noexcept : _value(p_initialValue) {}

		void mix(std::string_view p_value) noexcept
		{
			for (const char character : p_value)
			{
				_value ^= static_cast<std::uint8_t>(character);
				_value *= FnvPrime;
			}
		}

		// Integer mixing preserves Sparkle Playground's established seed format: the
		// canonical uint32 value is folded once, then multiplied by the FNV prime.
		void mix(std::int32_t p_value) noexcept
		{
			_value ^= static_cast<std::uint32_t>(p_value);
			_value *= FnvPrime;
		}

		[[nodiscard]] std::uint64_t value() const noexcept { return _value; }
	};

	inline void mix(std::uint64_t &p_hash, std::string_view p_value) noexcept
	{
		StableHasher64 hasher(p_hash);
		hasher.mix(p_value);
		p_hash = hasher.value();
	}

	inline void mix(std::uint64_t &p_hash, int p_value) noexcept
	{
		StableHasher64 hasher(p_hash);
		hasher.mix(static_cast<std::int32_t>(p_value));
		p_hash = hasher.value();
	}

	[[nodiscard]] inline std::uint64_t fnv1a(std::string_view p_value) noexcept
	{
		StableHasher64 hasher;
		hasher.mix(p_value);
		return hasher.value();
	}

	[[nodiscard]] inline std::uint64_t deriveSeed(std::uint64_t p_masterSeed, std::string_view p_semanticPath)
	{
		return fnv1a(std::to_string(p_masterSeed) + "::" + std::string(p_semanticPath));
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
