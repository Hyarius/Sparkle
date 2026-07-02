#include "world/chunk_coordinates.hpp"

namespace
{
	[[nodiscard]] constexpr int floorDivide(int p_value, int p_divisor) noexcept
	{
		const int quotient = p_value / p_divisor;
		const int remainder = p_value % p_divisor;
		return remainder < 0 ? quotient - 1 : quotient;
	}
}

namespace pg
{
	ChunkCoordinates ChunkCoordinates::fromWorldCell(const spk::Vector3Int &p_cell) noexcept
	{
		return {{floorDivide(p_cell.x, 16), floorDivide(p_cell.y, 16), floorDivide(p_cell.z, 16)}};
	}

	spk::Vector3Int ChunkCoordinates::worldOrigin() const noexcept
	{
		return value * 16;
	}

	spk::Vector3Int ChunkCoordinates::localFromWorld(const spk::Vector3Int &p_cell) const noexcept
	{
		return p_cell - worldOrigin();
	}
}
