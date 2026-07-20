#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "structures/container/spk_grid_2d.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class PoissonDisk
	{
	public:
		using Point = spk::Vector2;
		using Size = spk::Vector2;
		using PointPredicate = std::function<bool(const Point &)>;

		struct Parameters
		{
			Size size = {0.0f, 0.0f};
			float radius = 1.0f;

			// 0 means unlimited.
			std::size_t maxPointCount = 0;

			int triesPerPoint = 30;
			std::uint32_t seed = 0;

			PointPredicate accept = nullptr;
		};

	public:
		[[nodiscard]] static std::vector<Point> generate(const Parameters &p_parameters);

		[[nodiscard]] static float radiusForTargetCount(
			Size p_size,
			std::size_t p_targetCount,
			float p_packingDensity = 0.70f);

		[[nodiscard]] static std::vector<Point> generateApproximateCount(
			Size p_size,
			std::size_t p_targetCount,
			float p_packingDensity = 0.70f,
			int p_triesPerPoint = 30,
			std::uint32_t p_seed = 0,
			PointPredicate p_accept = nullptr);

		template <typename TType, typename Predicate>
		[[nodiscard]] PointPredicate makePredicate(
			const Grid2D<TType> &p_grid,
			Predicate p_acceptCell)
		{
			return [&p_grid, p_acceptCell = std::move(p_acceptCell)](const PoissonDisk::Point &p_point) -> bool {
				if (p_point.x < 0.0f || p_point.y < 0.0f)
				{
					return false;
				}

				const std::size_t x = static_cast<std::size_t>(p_point.x);
				const std::size_t y = static_cast<std::size_t>(p_point.y);

				if (x >= p_grid.width() || y >= p_grid.height())
				{
					return false;
				}

				return std::invoke(p_acceptCell, p_grid(x, y));
			};
		}
	};
}