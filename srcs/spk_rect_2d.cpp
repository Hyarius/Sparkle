#include "spk_rect_2d.hpp"

namespace spk
{
	Rect2D::Rect2D(const Rect2D::Anchor &p_anchor, const Rect2D::Size &p_size) : anchor(p_anchor),
																				 size(p_size)
	{
	}

	Rect2D::Rect2D(const Rect2D::Anchor &p_anchor, const std::size_t p_width, const std::size_t p_height) : anchor(p_anchor),
																											size(static_cast<Size::value_type>(p_width), static_cast<Size::value_type>(p_height))
	{
	}

	Rect2D::Rect2D(const int p_x, const int p_y, const Rect2D::Size &p_size) : anchor(p_x, p_y),
																			   size(p_size)
	{
	}

	Rect2D::Rect2D(const int p_x, const int p_y, const std::size_t p_width, const std::size_t p_height) : anchor(p_x, p_y),
																										  size(static_cast<Size::value_type>(p_width), static_cast<Size::value_type>(p_height))
	{
	}

	int Rect2D::x() const noexcept
	{
		return anchor.x;
	}

	int Rect2D::y() const noexcept
	{
		return anchor.y;
	}

	std::uint32_t Rect2D::width() const noexcept
	{
		return size.x;
	}

	std::uint32_t Rect2D::height() const noexcept
	{
		return size.y;
	}

	int Rect2D::left() const noexcept
	{
		return anchor.x;
	}

	int Rect2D::top() const noexcept
	{
		return anchor.y;
	}

	int Rect2D::right() const noexcept
	{
		return anchor.x + static_cast<int>(size.x);
	}

	int Rect2D::bottom() const noexcept
	{
		return anchor.y + static_cast<int>(size.y);
	}

	bool Rect2D::empty() const noexcept
	{
		return (size.x == 0 || size.y == 0);
	}

	Rect2D Rect2D::atOrigin() const noexcept
	{
		return Rect2D(Anchor::Zero, size);
	}

	bool Rect2D::contains(const spk::Vector2Int &p_point) const noexcept
	{
		return (p_point.x >= left() && p_point.x < right()) &&
			   (p_point.y >= top() && p_point.y < bottom());
	}

	bool Rect2D::contains(const Rect2D &p_other) const noexcept
	{
		return p_other.left() >= left() &&
			   p_other.top() >= top() &&
			   p_other.right() <= right() &&
			   p_other.bottom() <= bottom();
	}

	Rect2D Rect2D::translated(const spk::Vector2Int &p_offset) const noexcept
	{
		return Rect2D(anchor + p_offset, size);
	}

	Rect2D Rect2D::shrink(const spk::Vector2Int &p_offset) const
	{
		const int newLeft = left() + p_offset.x;
		const int newTop = top() + p_offset.y;
		const int newRight = std::max(newLeft, right() - p_offset.x);
		const int newBottom = std::max(newTop, bottom() - p_offset.y);

		return Rect2D(
			newLeft,
			newTop,
			static_cast<std::size_t>(newRight - newLeft),
			static_cast<std::size_t>(newBottom - newTop));
	}

	Rect2D Rect2D::intersect(const Rect2D &p_other) const noexcept
	{
		const int intersectLeft = std::max(left(), p_other.left());
		const int intersectTop = std::max(top(), p_other.top());
		const int intersectRight = std::min(right(), p_other.right());
		const int intersectBottom = std::min(bottom(), p_other.bottom());

		if (intersectRight <= intersectLeft || intersectBottom <= intersectTop)
		{
			return Rect2D(intersectLeft, intersectTop, 0, 0);
		}

		return Rect2D(
			intersectLeft,
			intersectTop,
			static_cast<std::size_t>(intersectRight - intersectLeft),
			static_cast<std::size_t>(intersectBottom - intersectTop));
	}

	std::string Rect2D::toString() const
	{
		std::ostringstream outputStream;
		outputStream << *this;
		return outputStream.str();
	}

	bool Rect2D::operator==(const Rect2D &p_other) const noexcept
	{
		return anchor == p_other.anchor && size == p_other.size;
	}

	bool Rect2D::operator!=(const Rect2D &p_other) const noexcept
	{
		return !(*this == p_other);
	}
}