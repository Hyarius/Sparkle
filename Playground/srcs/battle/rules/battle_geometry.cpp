#include "battle/rules/battle_geometry.hpp"

#include <algorithm>
#include <cmath>

namespace
{
	[[nodiscard]] int sign(int p_value) noexcept
	{
		return (p_value > 0) - (p_value < 0);
	}
}

namespace pg::BattleGeometry
{
	bool rangeContains(
		RangeShape p_shape,
		const spk::Vector2Int &p_offset,
		int p_minimumRange,
		int p_maximumRange) noexcept
	{
		const int absoluteX = std::abs(p_offset.x);
		const int absoluteZ = std::abs(p_offset.y);
		switch (p_shape)
		{
		case RangeShape::Self:
			return absoluteX == 0 && absoluteZ == 0;
		case RangeShape::Circle:
		{
			const int distance = absoluteX + absoluteZ;
			return distance >= p_minimumRange && distance <= p_maximumRange;
		}
		case RangeShape::Line:
		{
			if ((absoluteX == 0) == (absoluteZ == 0))
			{
				return false; // both zero (origin) or both non-zero (off-axis)
			}
			const int distance = absoluteX + absoluteZ;
			return distance >= p_minimumRange && distance <= p_maximumRange;
		}
		case RangeShape::Diagonal:
		{
			if (absoluteX == 0 || absoluteX != absoluteZ)
			{
				return false;
			}
			return absoluteX >= p_minimumRange && absoluteX <= p_maximumRange;
		}
		}
		return false;
	}

	std::vector<spk::Vector2Int> areaOffsets(
		AreaShape p_shape,
		int p_value,
		const spk::Vector2Int &p_casterToAnchor)
	{
		std::vector<spk::Vector2Int> result;
		const int value = std::max(0, p_value);
		switch (p_shape)
		{
		case AreaShape::Square:
			for (int z = -value; z <= value; ++z)
			{
				for (int x = -value; x <= value; ++x)
				{
					result.push_back({x, z});
				}
			}
			break;
		case AreaShape::Cross:
			for (int z = -value; z <= value; ++z)
			{
				for (int x = -value; x <= value; ++x)
				{
					if ((x == 0 || z == 0) && std::abs(x) + std::abs(z) <= value)
					{
						result.push_back({x, z});
					}
				}
			}
			break;
		case AreaShape::Circle:
			for (int z = -value; z <= value; ++z)
			{
				for (int x = -value; x <= value; ++x)
				{
					if (std::abs(x) + std::abs(z) <= value)
					{
						result.push_back({x, z});
					}
				}
			}
			break;
		case AreaShape::Line:
		{
			spk::Vector2Int step{0, 0};
			if (std::abs(p_casterToAnchor.x) >= std::abs(p_casterToAnchor.y))
			{
				step.x = sign(p_casterToAnchor.x);
			}
			else
			{
				step.y = sign(p_casterToAnchor.y);
			}
			result.push_back({0, 0});
			if (step.x != 0 || step.y != 0)
			{
				for (int k = 1; k <= value; ++k)
				{
					result.push_back({step.x * k, step.y * k});
				}
			}
			break;
		}
		}
		return result;
	}
}
