#pragma once

#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <tuple>

namespace pg
{
	struct ChunkCoordinates
	{
		spk::Vector3Int value{};

		[[nodiscard]] static ChunkCoordinates fromWorldCell(const spk::Vector3Int &p_cell) noexcept;
		[[nodiscard]] spk::Vector3Int worldOrigin() const noexcept;
		[[nodiscard]] spk::Vector3Int localFromWorld(const spk::Vector3Int &p_cell) const noexcept;

		[[nodiscard]] bool operator==(const ChunkCoordinates &) const noexcept = default;
		[[nodiscard]] bool operator<(const ChunkCoordinates &p_other) const noexcept
		{
			return std::tie(value.x, value.y, value.z) <
				   std::tie(p_other.value.x, p_other.value.y, p_other.value.z);
		}
	};
}

namespace std
{
	template <>
	struct hash<pg::ChunkCoordinates>
	{
		[[nodiscard]] std::size_t operator()(const pg::ChunkCoordinates &p_coordinates) const noexcept
		{
			std::size_t result = std::hash<std::int32_t>{}(p_coordinates.value.x);
			result ^= std::hash<std::int32_t>{}(p_coordinates.value.y) + 0x9e3779b9u + (result << 6) + (result >> 2);
			result ^= std::hash<std::int32_t>{}(p_coordinates.value.z) + 0x9e3779b9u + (result << 6) + (result >> 2);
			return result;
		}
	};
}
