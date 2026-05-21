#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace spk
{
	class BinaryField
	{
	private:
		enum class Kind
		{
			Value,
			Object,
			Array
		};

		static constexpr std::size_t invalidSectionID = static_cast<std::size_t>(-1);

		struct Section
		{
			std::string name;
			Kind kind = Kind::Value;

			std::uint8_t* data = nullptr;
			std::size_t size = 0;
			std::size_t offset = 0;

			std::size_t elementSize = 0;
			std::size_t count = 0;

			std::size_t parent = invalidSectionID;
			std::vector<std::size_t> children;
		};

		std::shared_ptr<std::vector<Section>> _sections;
		std::size_t _sectionID = invalidSectionID;

		BinaryField(const std::shared_ptr<std::vector<Section>>& p_sections, std::size_t p_sectionID);

		Section& _section();
		const Section& _section() const;

		std::size_t _findChild(std::string_view p_name) const;
		BinaryField _addSection(std::string_view p_name, std::size_t p_offset, std::size_t p_size, Kind p_kind);

		template <typename TValueType>
		void _writeExact(const TValueType& p_value)
		{
			static_assert(std::is_trivially_copyable_v<TValueType>);

			Section& section = _section();
			if (sizeof(TValueType) != section.size)
			{
				throw std::runtime_error("BinaryField assignment received a value with the wrong size.");
			}

			std::memcpy(section.data, &p_value, sizeof(TValueType));
		}

	public:
		BinaryField() = default;
		BinaryField(std::uint8_t* p_data, std::size_t p_size);

		bool isValid() const;

		std::string_view name() const;
		std::size_t size() const;
		std::size_t offset() const;
		std::size_t count() const;
		std::size_t elementSize() const;
		std::uint8_t* data();
		const std::uint8_t* data() const;

		BinaryField addValue(std::string_view p_name, std::size_t p_offset, std::size_t p_size);
		BinaryField addObject(std::string_view p_name, std::size_t p_offset, std::size_t p_size);
		BinaryField addArray(std::string_view p_name, std::size_t p_offset, std::size_t p_count, std::size_t p_elementSize);

		BinaryField operator[](std::string_view p_name);
		BinaryField operator[](std::size_t p_index);

		template <typename TValueType>
		BinaryField& operator=(const TValueType& p_value)
		{
			_writeExact(p_value);
			return *this;
		}

		template <typename TValueType, std::size_t NSize>
		BinaryField& operator=(const std::array<TValueType, NSize>& p_values)
		{
			static_assert(std::is_trivially_copyable_v<TValueType>);

			Section& section = _section();
			if (sizeof(TValueType) * NSize != section.size)
			{
				throw std::runtime_error("BinaryField array assignment received values with the wrong size.");
			}

			std::memcpy(section.data, p_values.data(), sizeof(TValueType) * NSize);
			return *this;
		}

		template <typename TValueType>
		BinaryField& set(const TValueType& p_value)
		{
			_writeExact(p_value);
			return *this;
		}

		template <typename TValueType>
		TValueType as() const
		{
			static_assert(std::is_trivially_copyable_v<TValueType>);

			const Section& section = _section();
			if (sizeof(TValueType) != section.size)
			{
				throw std::runtime_error("BinaryField::as received a type with the wrong size.");
			}

			TValueType result;
			std::memcpy(&result, section.data, sizeof(TValueType));
			return result;
		}

		std::span<std::uint8_t> bytes();
		std::span<const std::uint8_t> bytes() const;
	};
}
