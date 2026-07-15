#include "board/board_cell.hpp"

#include <limits>

namespace pg
{
	namespace
	{
		// Widening to 64 bits and range-checking is the portable form of a checked add: the
		// operands are ints, so no int64 sum of two of them can itself overflow.
		[[nodiscard]] std::optional<int> narrow(std::int64_t p_value) noexcept
		{
			if (p_value < std::numeric_limits<int>::min() || p_value > std::numeric_limits<int>::max())
			{
				return std::nullopt;
			}
			return static_cast<int>(p_value);
		}
	}

	std::optional<int> tryAdd(int p_left, int p_right) noexcept
	{
		return narrow(static_cast<std::int64_t>(p_left) + static_cast<std::int64_t>(p_right));
	}

	std::optional<int> trySubtract(int p_left, int p_right) noexcept
	{
		return narrow(static_cast<std::int64_t>(p_left) - static_cast<std::int64_t>(p_right));
	}

	std::optional<spk::Vector3Int> tryAdd(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) noexcept
	{
		const std::optional<int> x = tryAdd(p_left.x, p_right.x);
		const std::optional<int> y = tryAdd(p_left.y, p_right.y);
		const std::optional<int> z = tryAdd(p_left.z, p_right.z);
		if (!x.has_value() || !y.has_value() || !z.has_value())
		{
			return std::nullopt;
		}
		return spk::Vector3Int{*x, *y, *z};
	}

	std::optional<spk::Vector3Int> trySubtract(const spk::Vector3Int &p_left, const spk::Vector3Int &p_right) noexcept
	{
		const std::optional<int> x = trySubtract(p_left.x, p_right.x);
		const std::optional<int> y = trySubtract(p_left.y, p_right.y);
		const std::optional<int> z = trySubtract(p_left.z, p_right.z);
		if (!x.has_value() || !y.has_value() || !z.has_value())
		{
			return std::nullopt;
		}
		return spk::Vector3Int{*x, *y, *z};
	}
}
