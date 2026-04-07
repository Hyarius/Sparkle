#pragma once

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "spk_vector2.hpp"

namespace spk
{
	struct Rect2D
	{
		using Anchor = spk::Vector2Int;
		using Size = spk::Vector2UInt;

		Anchor anchor{};
		Size size{};

		Rect2D() = default;

		Rect2D(const Anchor& p_anchor, const Size& p_size);

		Rect2D(const Anchor& p_anchor, const std::size_t p_width, const std::size_t p_height);

		Rect2D(const int p_x, const int p_y, const Size& p_size);

		Rect2D(const int p_x, const int p_y, const std::size_t p_width, const std::size_t p_height);

		[[nodiscard]] int x() const noexcept;
		[[nodiscard]] int y() const noexcept;
		[[nodiscard]] std::uint32_t width() const noexcept;
		[[nodiscard]] std::uint32_t height() const noexcept;

		[[nodiscard]] int left() const noexcept;
		[[nodiscard]] int top() const noexcept;
		[[nodiscard]] int right() const noexcept;
		[[nodiscard]] int bottom() const noexcept;

		[[nodiscard]] bool empty() const noexcept;

		[[nodiscard]] Rect2D atOrigin() const noexcept;

		[[nodiscard]] bool contains(const spk::Vector2Int& p_point) const noexcept;
		[[nodiscard]] bool contains(const Rect2D& p_other) const noexcept;

		[[nodiscard]] Rect2D translated(const spk::Vector2Int& p_offset) const noexcept;
		[[nodiscard]] Rect2D shrink(const spk::Vector2Int& p_offset) const;
		[[nodiscard]] Rect2D intersect(const Rect2D& p_other) const noexcept;

		[[nodiscard]] std::string toString() const;
		friend std::ostream& operator<<(std::ostream& p_outputStream, const Rect2D& p_extend)
		{
			p_outputStream << "{ anchor: " << p_extend.anchor << ", size: " << p_extend.size << " }";
			return p_outputStream;
		}

		[[nodiscard]] bool operator==(const Rect2D& p_other) const noexcept;
		[[nodiscard]] bool operator!=(const Rect2D& p_other) const noexcept;
	};
}