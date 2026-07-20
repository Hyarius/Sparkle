#pragma once

#include <cstdint>
#include <string_view>

namespace spk::deterministic
{
	/**
	 * Deterministic, non-cryptographic hashing utilities.
	 *
	 * Their outputs are independent of std::hash, but are sensitive to every format
	 * detail below. Persisted seeds and procedural-content formats must version their
	 * domain identifiers. These utilities are not suitable for passwords, signatures,
	 * authentication, or adversarial collision resistance. Strings are mixed as their
	 * exact bytes; callers needing Unicode semantics must normalize before calling.
	 */
	namespace fnv1a64
	{
		inline constexpr std::uint64_t OffsetBasis = 14695981039346656037ULL;
		inline constexpr std::uint64_t Prime = 1099511628211ULL;

		/** Hashes the exact bytes in p_value with the standard FNV-1a 64-bit algorithm. */
		[[nodiscard]] constexpr std::uint64_t hash(std::string_view p_value) noexcept
		{
			std::uint64_t result = OffsetBasis;
			for (const char character : p_value)
			{
				result ^= static_cast<std::uint8_t>(character);
				result *= Prime;
			}
			return result;
		}
	}

	/**
	 * Incrementally hashes structured values using FNV-1a 64-bit state transitions.
	 * Each mix call encodes [one-byte type tag][eight-byte little-endian byte count]
	 * followed by its canonical bytes. Integer bytes are fixed-width little-endian;
	 * signed values use their modulo-2^N unsigned representation. No object memory,
	 * platform byte order, or implementation-defined integer width is used.
	 */
	class StableHasher64
	{
	private:
		enum class ValueTag : std::uint8_t
		{
			Domain = 0,
			String = 1,
			UInt32 = 2,
			Int32 = 3,
			UInt64 = 4,
			Int64 = 5
		};

		std::uint64_t _value = fnv1a64::OffsetBasis;

		constexpr void _mixByte(std::uint8_t p_byte) noexcept
		{
			_value ^= p_byte;
			_value *= fnv1a64::Prime;
		}

		constexpr void _mixUnsignedLittleEndian(std::uint64_t p_value, std::uint8_t p_byteCount) noexcept
		{
			for (std::uint8_t index = 0; index < p_byteCount; ++index)
			{
				_mixByte(static_cast<std::uint8_t>(p_value >> (index * 8U)));
			}
		}

		constexpr void _mixHeader(ValueTag p_tag, std::uint64_t p_byteCount) noexcept
		{
			_mixByte(static_cast<std::uint8_t>(p_tag));
			_mixUnsignedLittleEndian(p_byteCount, 8);
		}

		constexpr void _mixString(ValueTag p_tag, std::string_view p_value) noexcept
		{
			_mixHeader(p_tag, static_cast<std::uint64_t>(p_value.size()));
			for (const char character : p_value)
			{
				_mixByte(static_cast<std::uint8_t>(character));
			}
		}

	public:
		constexpr StableHasher64() = default;

		explicit constexpr StableHasher64(std::uint64_t p_initialValue) noexcept :
			_value(p_initialValue)
		{
		}

		/** Adds a versioned domain identifier, distinct from an ordinary string value. */
		constexpr void mixDomain(std::string_view p_value) noexcept
		{
			_mixString(ValueTag::Domain, p_value);
		}

		constexpr void mix(std::string_view p_value) noexcept
		{
			_mixString(ValueTag::String, p_value);
		}

		constexpr void mix(std::uint32_t p_value) noexcept
		{
			_mixHeader(ValueTag::UInt32, 4);
			_mixUnsignedLittleEndian(p_value, 4);
		}

		constexpr void mix(std::int32_t p_value) noexcept
		{
			_mixHeader(ValueTag::Int32, 4);
			_mixUnsignedLittleEndian(static_cast<std::uint32_t>(p_value), 4);
		}

		constexpr void mix(std::uint64_t p_value) noexcept
		{
			_mixHeader(ValueTag::UInt64, 8);
			_mixUnsignedLittleEndian(p_value, 8);
		}

		constexpr void mix(std::int64_t p_value) noexcept
		{
			_mixHeader(ValueTag::Int64, 8);
			_mixUnsignedLittleEndian(static_cast<std::uint64_t>(p_value), 8);
		}

		[[nodiscard]] constexpr std::uint64_t value() const noexcept
		{
			return _value;
		}
	};

	/**
	 * Derives a seed from the versioned domain "spk.derive-seed.v1", a canonical
	 * uint64 master seed, and a byte-string semantic path. This is a format migration;
	 * it intentionally does not preserve the former decimal-text seed encoding.
	 */
	[[nodiscard]] constexpr std::uint64_t deriveSeed(
		std::uint64_t p_masterSeed,
		std::string_view p_semanticPath) noexcept;

	/**
	 * The SplitMix64 finalizer: a deterministic non-cryptographic bit mixer, not a
	 * random-number generator by itself. Its exact output is part of this contract.
	 */
	[[nodiscard]] constexpr std::uint64_t splitMix64Finalize(std::uint64_t p_value) noexcept
	{
		p_value ^= p_value >> 30U;
		p_value *= 0xbf58476d1ce4e5b9ULL;
		p_value ^= p_value >> 27U;
		p_value *= 0x94d049bb133111ebULL;
		p_value ^= p_value >> 31U;
		return p_value;
	}

	constexpr std::uint64_t deriveSeed(std::uint64_t p_masterSeed, std::string_view p_semanticPath) noexcept
	{
		StableHasher64 hasher;
		hasher.mixDomain("spk.derive-seed.v1");
		hasher.mix(p_masterSeed);
		hasher.mix(p_semanticPath);
		return splitMix64Finalize(hasher.value());
	}

	/**
	 * Converts the upper 53 input bits to a binary64 grid point in [0.0, 1.0).
	 * The lower 11 bits are discarded and this function does not finalize the input.
	 */
	[[nodiscard]] constexpr double toUnitInterval(std::uint64_t p_value) noexcept
	{
		return static_cast<double>(p_value >> 11U) * (1.0 / 9007199254740992.0);
	}
}
