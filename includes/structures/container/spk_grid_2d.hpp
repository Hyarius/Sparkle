#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "structures/math/spk_vector2.hpp"

namespace spk
{
	template <typename TType>
	class Grid2D
	{
	public:
		using Value = TType;
		using Storage = std::vector<TType>;
		using reference = typename Storage::reference;
		using const_reference = typename Storage::const_reference;

	private:
		spk::Vector2UInt _size = {0, 0};
		Storage _data;

		[[nodiscard]] static std::size_t _elementCount(const spk::Vector2UInt &p_size)
		{
			static_assert(sizeof(std::size_t) >= sizeof(std::uint64_t));

			const std::size_t width = static_cast<std::size_t>(p_size.x);
			const std::size_t height = static_cast<std::size_t>(p_size.y);

			const std::size_t count = width * height;
			if (count > Storage().max_size())
			{
				throw std::length_error("spk::Grid2D dimensions are too large");
			}

			return count;
		}

	public:
		Grid2D() = default;

		Grid2D(spk::Vector2UInt p_size, const TType &p_fill = TType{})
		{
			resize(p_size, p_fill);
		}

		[[nodiscard]] Storage& data()
		{
			return _data;
		}

		[[nodiscard]] const Storage& data() const
		{
			return _data;
		}

		[[nodiscard]] spk::Vector2UInt size() const noexcept
		{
			return _size;
		}

		[[nodiscard]] std::size_t width() const noexcept
		{
			return static_cast<std::size_t>(_size.x);
		}

		[[nodiscard]] std::size_t height() const noexcept
		{
			return static_cast<std::size_t>(_size.y);
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return _data.empty();
		}

		[[nodiscard]] bool contains(std::size_t p_x, std::size_t p_y) const noexcept
		{
			return p_x < width() && p_y < height();
		}

		[[nodiscard]] std::size_t indexOf(std::size_t p_x, std::size_t p_y) const
		{
			if (contains(p_x, p_y) == false)
			{
				throw std::out_of_range("spk::Grid2D coordinates are out of range");
			}

			return p_y * width() + p_x;
		}

		[[nodiscard]] TType &at(std::size_t p_x, std::size_t p_y)
		{
			return _data[indexOf(p_x, p_y)];
		}

		[[nodiscard]] const TType &at(std::size_t p_x, std::size_t p_y) const
		{
			return _data[indexOf(p_x, p_y)];
		}

		[[nodiscard]] TType &operator()(std::size_t p_x, std::size_t p_y)
		{
			return at(p_x, p_y);
		}

		[[nodiscard]] const TType &operator()(std::size_t p_x, std::size_t p_y) const
		{
			return at(p_x, p_y);
		}

		[[nodiscard]] reference operator[](std::size_t p_x, std::size_t p_y)
		{
			return at(p_x, p_y);
		}

		[[nodiscard]] const_reference operator[](std::size_t p_x, std::size_t p_y) const
		{
			return at(p_x, p_y);
		}

		void fill(const TType &p_value)
		{
			std::fill(_data.begin(), _data.end(), p_value);
		}

		void resize(spk::Vector2UInt p_size, const TType &p_fill = TType{})
		{
			Storage newData(_elementCount(p_size), p_fill);
			_data.swap(newData);
			_size = p_size;
		}
	};
}
