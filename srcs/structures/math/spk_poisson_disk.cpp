#include "structures/math/spk_poisson_disk.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	constexpr float PI = 3.14159265358979323846f;
	constexpr float TWO_PI = 2.0f * PI;
	constexpr int INITIAL_POINT_ATTEMPTS = 10'000;

	struct Cell
	{
		std::size_t x = 0;
		std::size_t y = 0;
	};

	class RandomGenerator
	{
	private:
		std::uint64_t _state = 0;

	public:
		explicit RandomGenerator(std::uint32_t p_seed) noexcept :
			_state(static_cast<std::uint64_t>(p_seed))
		{
		}

		[[nodiscard]] std::uint64_t nextU64() noexcept
		{
			_state += 0x9E3779B97F4A7C15ull;

			std::uint64_t result = _state;

			result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull;
			result = (result ^ (result >> 27)) * 0x94D049BB133111EBull;
			result = result ^ (result >> 31);

			return result;
		}

		[[nodiscard]] float nextFloat01() noexcept
		{
			const std::uint64_t value = nextU64() >> 40;

			return static_cast<float>(value) * (1.0f / 16777216.0f);
		}

		[[nodiscard]] float range(float p_min, float p_max) noexcept
		{
			return p_min + (p_max - p_min) * nextFloat01();
		}

		[[nodiscard]] std::size_t index(std::size_t p_exclusiveMaximum) noexcept
		{
			return static_cast<std::size_t>(nextU64() % p_exclusiveMaximum);
		}
	};

	[[nodiscard]] bool isFinite(float p_value) noexcept
	{
		return std::isfinite(p_value);
	}

	[[nodiscard]] bool isFinite(const spk::PoissonDisk::Point &p_point) noexcept
	{
		return isFinite(p_point.x) && isFinite(p_point.y);
	}

	void validateSize(const spk::PoissonDisk::Size &p_size)
	{
		if (isFinite(p_size) == false || p_size.x <= 0.0f || p_size.y <= 0.0f)
		{
			throw std::invalid_argument("spk::PoissonDisk size must be finite and greater than zero");
		}
	}

	void validateParameters(const spk::PoissonDisk::Parameters &p_parameters)
	{
		validateSize(p_parameters.size);

		if (isFinite(p_parameters.radius) == false || p_parameters.radius <= 0.0f)
		{
			throw std::invalid_argument("spk::PoissonDisk radius must be finite and greater than zero");
		}

		if (p_parameters.triesPerPoint <= 0)
		{
			throw std::invalid_argument("spk::PoissonDisk triesPerPoint must be greater than zero");
		}
	}

	void validatePackingDensity(float p_packingDensity)
	{
		if (isFinite(p_packingDensity) == false || p_packingDensity <= 0.0f || p_packingDensity > 1.0f)
		{
			throw std::invalid_argument("spk::PoissonDisk packingDensity must be finite and inside ]0, 1]");
		}
	}

	[[nodiscard]] bool hasReachedMaxPointCount(
		std::size_t p_currentCount,
		std::size_t p_maxPointCount) noexcept
	{
		return p_maxPointCount != 0 && p_currentCount >= p_maxPointCount;
	}

	[[nodiscard]] bool isInside(
		const spk::PoissonDisk::Point &p_point,
		const spk::PoissonDisk::Size &p_size) noexcept
	{
		return p_point.x >= 0.0f &&
			   p_point.y >= 0.0f &&
			   p_point.x < p_size.x &&
			   p_point.y < p_size.y;
	}

	[[nodiscard]] bool isAccepted(
		const spk::PoissonDisk::Point &p_point,
		const spk::PoissonDisk::Parameters &p_parameters)
	{
		if (isInside(p_point, p_parameters.size) == false)
		{
			return false;
		}

		if (p_parameters.accept != nullptr && p_parameters.accept(p_point) == false)
		{
			return false;
		}

		return true;
	}

	[[nodiscard]] Cell cellOf(
		const spk::PoissonDisk::Point &p_point,
		float p_cellSize,
		std::size_t p_gridWidth,
		std::size_t p_gridHeight) noexcept
	{
		const std::size_t x = std::min(
			static_cast<std::size_t>(p_point.x / p_cellSize),
			p_gridWidth - 1);

		const std::size_t y = std::min(
			static_cast<std::size_t>(p_point.y / p_cellSize),
			p_gridHeight - 1);

		return Cell{x, y};
	}

	[[nodiscard]] std::size_t gridIndex(
		Cell p_cell,
		std::size_t p_gridWidth) noexcept
	{
		return p_cell.y * p_gridWidth + p_cell.x;
	}

	[[nodiscard]] bool isFarEnough(
		const spk::PoissonDisk::Point &p_candidate,
		const std::vector<spk::PoissonDisk::Point> &p_points,
		const std::vector<int> &p_grid,
		std::size_t p_gridWidth,
		std::size_t p_gridHeight,
		float p_cellSize,
		float p_radius)
	{
		const Cell candidateCell = cellOf(p_candidate, p_cellSize, p_gridWidth, p_gridHeight);

		const std::size_t searchRadius = static_cast<std::size_t>(std::ceil(p_radius / p_cellSize));

		const std::size_t minX = candidateCell.x > searchRadius ? candidateCell.x - searchRadius : 0;
		const std::size_t minY = candidateCell.y > searchRadius ? candidateCell.y - searchRadius : 0;

		const std::size_t maxX = std::min(candidateCell.x + searchRadius, p_gridWidth - 1);
		const std::size_t maxY = std::min(candidateCell.y + searchRadius, p_gridHeight - 1);

		const float radiusSquared = p_radius * p_radius;

		for (std::size_t y = minY; y <= maxY; ++y)
		{
			for (std::size_t x = minX; x <= maxX; ++x)
			{
				const int pointIndex = p_grid[gridIndex(Cell{x, y}, p_gridWidth)];

				if (pointIndex < 0)
				{
					continue;
				}

				const spk::PoissonDisk::Point &point = p_points[static_cast<std::size_t>(pointIndex)];

				const float dx = p_candidate.x - point.x;
				const float dy = p_candidate.y - point.y;

				if (dx * dx + dy * dy < radiusSquared)
				{
					return false;
				}
			}
		}

		return true;
	}

	[[nodiscard]] spk::PoissonDisk::Point randomPoint(
		const spk::PoissonDisk::Size &p_size,
		RandomGenerator &p_random) noexcept
	{
		return {
			p_random.range(0.0f, p_size.x),
			p_random.range(0.0f, p_size.y)};
	}

	[[nodiscard]] bool findInitialPoint(
		const spk::PoissonDisk::Parameters &p_parameters,
		RandomGenerator &p_random,
		spk::PoissonDisk::Point &p_result)
	{
		for (int i = 0; i < INITIAL_POINT_ATTEMPTS; ++i)
		{
			const spk::PoissonDisk::Point candidate = randomPoint(p_parameters.size, p_random);

			if (isAccepted(candidate, p_parameters))
			{
				p_result = candidate;
				return true;
			}
		}

		return false;
	}

	[[nodiscard]] spk::PoissonDisk::Point randomCandidateAround(
		const spk::PoissonDisk::Point &p_center,
		float p_radius,
		RandomGenerator &p_random) noexcept
	{
		const float angle = p_random.range(0.0f, TWO_PI);

		const float innerSquared = p_radius * p_radius;
		const float outerSquared = 4.0f * innerSquared;

		const float distance = std::sqrt(
			p_random.range(innerSquared, outerSquared));

		return {
			p_center.x + std::cos(angle) * distance,
			p_center.y + std::sin(angle) * distance};
	}

	[[nodiscard]] std::size_t checkedGridCellCount(
		std::size_t p_gridWidth,
		std::size_t p_gridHeight)
	{
		if (p_gridHeight > std::numeric_limits<std::size_t>::max() / p_gridWidth)
		{
			throw std::length_error("spk::PoissonDisk internal grid is too large");
		}

		return p_gridWidth * p_gridHeight;
	}
}

namespace spk
{
	std::vector<PoissonDisk::Point> PoissonDisk::generate(const Parameters &p_parameters)
	{
		validateParameters(p_parameters);

		const float cellSize = p_parameters.radius / std::sqrt(2.0f);

		const std::size_t gridWidth = static_cast<std::size_t>(
			std::ceil(p_parameters.size.x / cellSize));

		const std::size_t gridHeight = static_cast<std::size_t>(
			std::ceil(p_parameters.size.y / cellSize));

		const std::size_t gridCellCount = checkedGridCellCount(gridWidth, gridHeight);

		std::vector<int> grid(gridCellCount, -1);
		std::vector<Point> points;
		std::vector<std::size_t> activePoints;

		RandomGenerator random(p_parameters.seed);

		Point initialPoint;

		if (findInitialPoint(p_parameters, random, initialPoint) == false)
		{
			return points;
		}

		points.push_back(initialPoint);
		activePoints.push_back(0);

		const Cell initialCell = cellOf(initialPoint, cellSize, gridWidth, gridHeight);
		grid[gridIndex(initialCell, gridWidth)] = 0;

		if (hasReachedMaxPointCount(points.size(), p_parameters.maxPointCount))
		{
			return points;
		}

		while (activePoints.empty() == false)
		{
			if (hasReachedMaxPointCount(points.size(), p_parameters.maxPointCount))
			{
				break;
			}

			const std::size_t activeIndex = random.index(activePoints.size());
			const std::size_t pointIndex = activePoints[activeIndex];
			const Point center = points[pointIndex];

			bool foundCandidate = false;

			for (int attempt = 0; attempt < p_parameters.triesPerPoint; ++attempt)
			{
				const Point candidate = randomCandidateAround(center, p_parameters.radius, random);

				if (isAccepted(candidate, p_parameters) == false)
				{
					continue;
				}

				if (isFarEnough(
						candidate,
						points,
						grid,
						gridWidth,
						gridHeight,
						cellSize,
						p_parameters.radius) == false)
				{
					continue;
				}

				const std::size_t newPointIndex = points.size();

				points.push_back(candidate);
				activePoints.push_back(newPointIndex);

				const Cell candidateCell = cellOf(candidate, cellSize, gridWidth, gridHeight);
				grid[gridIndex(candidateCell, gridWidth)] = static_cast<int>(newPointIndex);

				foundCandidate = true;
				break;
			}

			if (foundCandidate == false)
			{
				activePoints[activeIndex] = activePoints.back();
				activePoints.pop_back();
			}
		}

		return points;
	}

	float PoissonDisk::radiusForTargetCount(
		Size p_size,
		std::size_t p_targetCount,
		float p_packingDensity)
	{
		validateSize(p_size);
		validatePackingDensity(p_packingDensity);

		if (p_targetCount == 0)
		{
			throw std::invalid_argument("spk::PoissonDisk targetCount must be greater than zero");
		}

		const float area = p_size.x * p_size.y;

		return 2.0f * std::sqrt(
						  (area * p_packingDensity) /
						  (static_cast<float>(p_targetCount) * PI));
	}

	std::vector<PoissonDisk::Point> PoissonDisk::generateApproximateCount(
		Size p_size,
		std::size_t p_targetCount,
		float p_packingDensity,
		int p_triesPerPoint,
		std::uint32_t p_seed,
		PointPredicate p_accept)
	{
		if (p_targetCount == 0)
		{
			return {};
		}

		const float radius = radiusForTargetCount(p_size, p_targetCount, p_packingDensity);

		return generate({.size = p_size, .radius = radius, .maxPointCount = p_targetCount, .triesPerPoint = p_triesPerPoint, .seed = p_seed, .accept = std::move(p_accept)});
	}
}