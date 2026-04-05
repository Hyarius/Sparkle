#pragma once

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <type_traits>
#include <utility>

namespace spk
{
	template <typename TEnumType>
	concept enum_type =
		std::is_enum_v<TEnumType>;

	template <typename TType>
	concept unsigned_storage =
		std::is_unsigned_v<TType> &&
		(sizeof(TType) == 1 || sizeof(TType) == 2 || sizeof(TType) == 4);

	template <enum_type TFlagType, unsigned_storage TStorageType = std::uint32_t>
	class alignas(TStorageType) Flags
	{
		static_assert(sizeof(std::underlying_type_t<TFlagType>) <= sizeof(TStorageType), "Storage type too small for enum underlying type");

	public:
		using FlagType = TFlagType;
		using MaskType = TStorageType;

	private:
		MaskType _bits = 0;

		static constexpr MaskType _toMask(TFlagType p_value) noexcept
		{
			return static_cast<MaskType>(std::to_underlying(p_value));
		}

	public:
		constexpr Flags() = default;

		constexpr Flags(TFlagType p_value) noexcept :
			_bits(_toMask(p_value))
		{
		}

		constexpr explicit Flags(MaskType p_rawBits) noexcept :
			_bits(p_rawBits)
		{
		}

		constexpr Flags(std::initializer_list<TFlagType> p_values) noexcept
		{
			for (TFlagType value : p_values)
			{
				_bits |= _toMask(value);
			}
		}

		[[nodiscard]] constexpr MaskType raw() const noexcept
		{
			return _bits;
		}

		constexpr void clear() noexcept
		{
			_bits = 0;
		}

		constexpr void set(TFlagType p_value) noexcept
		{
			_bits |= _toMask(p_value);
		}

		constexpr void set(Flags p_other) noexcept
		{
			_bits |= p_other._bits;
		}

		constexpr void unset(TFlagType p_value) noexcept
		{
			_bits &= static_cast<MaskType>(~_toMask(p_value));
		}

		constexpr void unset(Flags p_other) noexcept
		{
			_bits &= static_cast<MaskType>(~p_other._bits);
		}

		constexpr void reset(TFlagType p_value) noexcept
		{
			unset(p_value);
		}

		constexpr void reset(Flags p_other) noexcept
		{
			unset(p_other);
		}

		constexpr void toggle(TFlagType p_value) noexcept
		{
			_bits ^= _toMask(p_value);
		}

		constexpr void toggle(Flags p_other) noexcept
		{
			_bits ^= p_other._bits;
		}

		[[nodiscard]] constexpr bool has(TFlagType p_value) const noexcept
		{
			const MaskType mask = _toMask(p_value);
			return (_bits & mask) == mask;
		}

		[[nodiscard]] constexpr bool has(Flags p_other) const noexcept
		{
			return (_bits & p_other._bits) == p_other._bits;
		}

		[[nodiscard]] constexpr bool testAny(TFlagType p_value) const noexcept
		{
			return (_bits & _toMask(p_value)) != 0;
		}

		[[nodiscard]] constexpr bool testAny(Flags p_other) const noexcept
		{
			return (_bits & p_other._bits) != 0;
		}

		[[nodiscard]] constexpr bool any() const noexcept
		{
			return _bits != 0;
		}

		[[nodiscard]] constexpr bool none() const noexcept
		{
			return _bits == 0;
		}

		[[nodiscard]] explicit constexpr operator bool() const noexcept
		{
			return any();
		}

		constexpr Flags& operator|=(TFlagType p_value) noexcept
		{
			set(p_value);
			return *this;
		}

		constexpr Flags& operator&=(TFlagType p_value) noexcept
		{
			_bits &= _toMask(p_value);
			return *this;
		}

		constexpr Flags& operator^=(TFlagType p_value) noexcept
		{
			toggle(p_value);
			return *this;
		}

		constexpr Flags& operator|=(Flags p_other) noexcept
		{
			_bits |= p_other._bits;
			return *this;
		}

		constexpr Flags& operator&=(Flags p_other) noexcept
		{
			_bits &= p_other._bits;
			return *this;
		}

		constexpr Flags& operator^=(Flags p_other) noexcept
		{
			_bits ^= p_other._bits;
			return *this;
		}

		friend constexpr Flags operator|(Flags p_left, Flags p_right) noexcept
		{
			p_left |= p_right;
			return p_left;
		}

		friend constexpr Flags operator&(Flags p_left, Flags p_right) noexcept
		{
			p_left &= p_right;
			return p_left;
		}

		friend constexpr Flags operator^(Flags p_left, Flags p_right) noexcept
		{
			p_left ^= p_right;
			return p_left;
		}

		friend constexpr Flags operator~(Flags p_value) noexcept
		{
			return Flags(static_cast<MaskType>(~static_cast<MaskType>(p_value._bits)));
		}

		friend constexpr Flags operator|(Flags p_left, TFlagType p_right) noexcept
		{
			p_left |= p_right;
			return p_left;
		}

		friend constexpr Flags operator|(TFlagType p_left, Flags p_right) noexcept
		{
			p_right |= p_left;
			return p_right;
		}

		friend constexpr Flags operator&(Flags p_left, TFlagType p_right) noexcept
		{
			p_left &= p_right;
			return p_left;
		}

		friend constexpr Flags operator&(TFlagType p_left, Flags p_right) noexcept
		{
			p_right &= p_left;
			return p_right;
		}

		friend constexpr Flags operator^(Flags p_left, TFlagType p_right) noexcept
		{
			p_left ^= p_right;
			return p_left;
		}

		friend constexpr Flags operator^(TFlagType p_left, Flags p_right) noexcept
		{
			p_right ^= p_left;
			return p_right;
		}

		friend constexpr bool operator==(Flags p_left, Flags p_right) noexcept = default;

		friend constexpr bool operator==(Flags p_left, TFlagType p_right) noexcept
		{
			return p_left._bits == _toMask(p_right);
		}

		friend constexpr bool operator==(TFlagType p_left, Flags p_right) noexcept
		{
			return p_right == p_left;
		}

		friend constexpr bool operator!=(Flags p_left, TFlagType p_right) noexcept
		{
			return !(p_left == p_right);
		}

		friend constexpr bool operator!=(TFlagType p_left, Flags p_right) noexcept
		{
			return !(p_right == p_left);
		}
	};

	template <enum_type TFlagType>
	using Flags8 = Flags<TFlagType, std::uint8_t>;

	template <enum_type TFlagType>
	using Flags16 = Flags<TFlagType, std::uint16_t>;

	template <enum_type TFlagType>
	using Flags32 = Flags<TFlagType, std::uint32_t>;
}

template <spk::enum_type TEnumType, spk::unsigned_storage TStorageType>
std::ostream& operator<<(std::ostream& p_outputStream, const spk::Flags<TEnumType, TStorageType>& p_flags)
{
	const std::ios_base::fmtflags previousFlags = p_outputStream.flags();
	p_outputStream << "0x" << std::hex << p_flags.raw();
	p_outputStream.flags(previousFlags);
	return p_outputStream;
}

template <spk::enum_type TEnumType, spk::unsigned_storage TStorageType>
std::wostream& operator<<(std::wostream& p_outputStream, const spk::Flags<TEnumType, TStorageType>& p_flags)
{
	const std::ios_base::fmtflags previousFlags = p_outputStream.flags();
	p_outputStream << L"0x" << std::hex << p_flags.raw();
	p_outputStream.flags(previousFlags);
	return p_outputStream;
}