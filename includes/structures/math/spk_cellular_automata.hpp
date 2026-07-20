#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>

#include "structures/container/spk_grid_2d.hpp"

namespace spk
{
	class CellularAutomata
	{
	public:
		enum class EdgePolicy
		{
			TreatOutsideAsStateA,
			TreatOutsideAsStateB,
			Wrap
		};

		struct Rule
		{
			std::size_t deathLimit = 3;
			std::size_t birthLimit = 4;
			EdgePolicy edgePolicy = EdgePolicy::TreatOutsideAsStateA;
		};

	private:
		inline static constexpr std::array<Vector2Int, 8> _neighborOffsets = {
			Vector2Int{-1, -1},
			Vector2Int{0, -1},
			Vector2Int{1, -1},
			Vector2Int{-1, 0},
			Vector2Int{1, 0},
			Vector2Int{-1, 1},
			Vector2Int{0, 1},
			Vector2Int{1, 1}};

		[[nodiscard]] static bool _tryOffsetCoordinate(
			std::size_t p_coordinate,
			std::size_t p_limit,
			int p_offset,
			std::size_t &p_result) noexcept
		{
			if (p_offset < 0)
			{
				if (p_coordinate == 0)
				{
					return false;
				}

				p_result = p_coordinate - 1;
				return true;
			}

			if (p_offset > 0)
			{
				if (p_coordinate + 1 >= p_limit)
				{
					return false;
				}

				p_result = p_coordinate + 1;
				return true;
			}

			p_result = p_coordinate;
			return p_coordinate < p_limit;
		}

		[[nodiscard]] static std::size_t _wrappedCoordinate(
			std::size_t p_coordinate,
			std::size_t p_limit,
			int p_offset) noexcept
		{
			if (p_offset < 0)
			{
				return p_coordinate == 0 ? p_limit - 1 : p_coordinate - 1;
			}

			if (p_offset > 0)
			{
				const std::size_t next = p_coordinate + 1;
				return next == p_limit ? 0 : next;
			}

			return p_coordinate;
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static bool _isWrappedNeighborStateA(
			const Grid2D<TType> &p_grid,
			std::size_t p_x,
			std::size_t p_y,
			const Vector2Int &p_offset,
			Predicate &p_isStateA)
		{
			const std::size_t neighborX = _wrappedCoordinate(p_x, p_grid.width(), p_offset.x);
			const std::size_t neighborY = _wrappedCoordinate(p_y, p_grid.height(), p_offset.y);

			return std::invoke(p_isStateA, p_grid(neighborX, neighborY));
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static bool _isBoundedNeighborStateA(
			const Grid2D<TType> &p_grid,
			std::size_t p_x,
			std::size_t p_y,
			const Vector2Int &p_offset,
			Predicate &p_isStateA,
			EdgePolicy p_edgePolicy)
		{
			std::size_t neighborX = 0;
			std::size_t neighborY = 0;

			const bool xIsInside = _tryOffsetCoordinate(p_x, p_grid.width(), p_offset.x, neighborX);
			const bool yIsInside = _tryOffsetCoordinate(p_y, p_grid.height(), p_offset.y, neighborY);

			if (xIsInside && yIsInside)
			{
				return std::invoke(p_isStateA, p_grid(neighborX, neighborY));
			}

			return p_edgePolicy == EdgePolicy::TreatOutsideAsStateA;
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static bool _isNeighborStateA(
			const Grid2D<TType> &p_grid,
			std::size_t p_x,
			std::size_t p_y,
			const Vector2Int &p_offset,
			Predicate &p_isStateA,
			EdgePolicy p_edgePolicy)
		{
			switch (p_edgePolicy)
			{
			case EdgePolicy::Wrap:
				return _isWrappedNeighborStateA(p_grid, p_x, p_y, p_offset, p_isStateA);

			default:
				return _isBoundedNeighborStateA(p_grid, p_x, p_y, p_offset, p_isStateA, p_edgePolicy);
			}
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static std::size_t _countStateANeighbors(
			const Grid2D<TType> &p_grid,
			std::size_t p_x,
			std::size_t p_y,
			Predicate &p_isStateA,
			EdgePolicy p_edgePolicy)
		{
			std::size_t result = 0;

			for (const Vector2Int &offset : _neighborOffsets)
			{
				if (_isNeighborStateA(p_grid, p_x, p_y, offset, p_isStateA, p_edgePolicy))
				{
					++result;
				}
			}

			return result;
		}

	public:
		CellularAutomata() = delete;

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		static void apply(
			Grid2D<TType> &p_grid,
			Predicate p_isStateA,
			const TType &p_stateA,
			const TType &p_stateB)

		{
			apply(p_grid, p_isStateA, p_stateA, p_stateB, CellularAutomata::Rule{});
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		static void apply(
			Grid2D<TType> &p_grid,
			Predicate p_isStateA,
			const TType &p_stateA,
			const TType &p_stateB,
			const CellularAutomata::Rule &p_rule)
		{
			p_grid = next(p_grid, p_isStateA, p_stateA, p_stateB, p_rule);
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static Grid2D<TType> next(
			const Grid2D<TType> &p_grid,
			Predicate p_isStateA,
			const TType &p_stateA,
			const TType &p_stateB)

		{
			return (next(p_grid, p_isStateA, p_stateA, p_stateB, CellularAutomata::Rule{}));
		}

		template <typename TType, typename Predicate>
			requires std::predicate<Predicate &, const TType &>
		[[nodiscard]] static Grid2D<TType> next(
			const Grid2D<TType> &p_grid,
			Predicate p_isStateA,
			const TType &p_stateA,
			const TType &p_stateB,
			const CellularAutomata::Rule &p_rule)
		{
			Grid2D<TType> result(p_grid.size(), p_stateB);

			for (std::size_t y = 0; y < p_grid.height(); ++y)
			{
				for (std::size_t x = 0; x < p_grid.width(); ++x)
				{
					const bool currentCellIsStateA = std::invoke(p_isStateA, p_grid(x, y));

					const std::size_t stateANeighborCount = _countStateANeighbors(
						p_grid,
						x,
						y,
						p_isStateA,
						p_rule.edgePolicy);

					if (currentCellIsStateA)
					{
						result(x, y) = stateANeighborCount < p_rule.deathLimit ? p_stateB : p_stateA;
					}
					else
					{
						result(x, y) = stateANeighborCount >= p_rule.birthLimit ? p_stateA : p_stateB;
					}
				}
			}

			return result;
		}
	};
}