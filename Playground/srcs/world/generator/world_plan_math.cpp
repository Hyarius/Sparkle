#include "world/generator/world_plan_math.hpp"

#include "core/deterministic_random.hpp"

#include <array>
#include <cmath>
#include <deque>
#include <limits>
#include <utility>

namespace pg::worldgen
{
	std::uint64_t deriveSeed(std::uint64_t p_master, const std::string &p_path)
	{
		return deterministic::deriveSeed(p_master, p_path);
	}

	Field valueNoise(Rng &p_rng, int p_size, double p_scale)
	{
		const int n = std::max(2, static_cast<int>(p_size / p_scale));
		std::vector<double> lattice(static_cast<std::size_t>(n + 2) * (n + 2));
		for (double &value : lattice)
		{
			value = p_rng.uniform();
		}
		const auto latticeAt = [&](int p_row, int p_col) {
			return lattice[static_cast<std::size_t>(p_row) * (n + 2) + p_col];
		};

		Field result(p_size);
		for (int i = 0; i < p_size; ++i)
		{
			const double y = static_cast<double>(i) * n / p_size;
			const int y0 = static_cast<int>(y);
			double fy = y - y0;
			fy = fy * fy * (3.0 - 2.0 * fy);
			for (int j = 0; j < p_size; ++j)
			{
				const double x = static_cast<double>(j) * n / p_size;
				const int x0 = static_cast<int>(x);
				double fx = x - x0;
				fx = fx * fx * (3.0 - 2.0 * fx);
				const double top = latticeAt(y0, x0) * (1.0 - fx) + latticeAt(y0, x0 + 1) * fx;
				const double bottom = latticeAt(y0 + 1, x0) * (1.0 - fx) + latticeAt(y0 + 1, x0 + 1) * fx;
				result.at(i, j) = top * (1.0 - fy) + bottom * fy;
			}
		}
		return result;
	}

	double perlinNoise(
		std::uint64_t p_seed,
		double p_worldX,
		double p_worldZ,
		double p_featureSize)
	{
		if (!std::isfinite(p_featureSize) || p_featureSize <= 0.0)
		{
			return 0.0;
		}

		const double x = p_worldX / p_featureSize;
		const double z = p_worldZ / p_featureSize;
		const auto x0 = static_cast<std::int64_t>(std::floor(x));
		const auto z0 = static_cast<std::int64_t>(std::floor(z));
		const double tx = x - static_cast<double>(x0);
		const double tz = z - static_cast<double>(z0);
		const auto fade = [](double p_value) {
			return p_value * p_value * p_value * (p_value * (p_value * 6.0 - 15.0) + 10.0);
		};
		const auto interpolate = [](double p_from, double p_to, double p_amount) {
			return p_from + (p_to - p_from) * p_amount;
		};
		const auto gradientDot = [p_seed](std::int64_t p_x, std::int64_t p_z, double p_dx, double p_dz) {
			// Axis and diagonal gradients keep the sampler inexpensive while the
			// avalanche removes visible correlation between neighboring lattice cells.
			static constexpr double diagonal = 0.70710678118654752440;
			static constexpr std::array<std::pair<double, double>, 8> gradients = {
				std::pair{1.0, 0.0},
				std::pair{-1.0, 0.0},
				std::pair{0.0, 1.0},
				std::pair{0.0, -1.0},
				std::pair{diagonal, diagonal},
				std::pair{-diagonal, diagonal},
				std::pair{diagonal, -diagonal},
				std::pair{-diagonal, -diagonal}};
			std::uint64_t hash = static_cast<std::uint64_t>(p_x);
				hash ^= deterministic::splitMix64Finalize(static_cast<std::uint64_t>(p_z) + 0x9e3779b97f4a7c15ULL);
				hash = deterministic::splitMix64Finalize(hash ^ p_seed);
			const auto &[gx, gz] = gradients[hash & 7U];
			return gx * p_dx + gz * p_dz;
		};

		const double top = interpolate(
			gradientDot(x0, z0, tx, tz),
			gradientDot(x0 + 1, z0, tx - 1.0, tz),
			fade(tx));
		const double bottom = interpolate(
			gradientDot(x0, z0 + 1, tx, tz - 1.0),
			gradientDot(x0 + 1, z0 + 1, tx - 1.0, tz - 1.0),
			fade(tx));
		// The theoretical extrema of this gradient set fit comfortably inside this
		// normalization. Clamp protects the public contract from round-off drift.
		return std::clamp(interpolate(top, bottom, fade(tz)) * 1.4142135623730950488, -1.0, 1.0);
	}

	double fractalPerlinNoise(
		std::uint64_t p_seed,
		double p_worldX,
		double p_worldZ,
		double p_featureSize,
		int p_octaves,
		double p_persistence)
	{
		if (!std::isfinite(p_featureSize) || p_featureSize <= 0.0 || !std::isfinite(p_persistence) ||
			p_persistence <= 0.0 || p_octaves <= 0)
		{
			return 0.0;
		}

		double total = 0.0;
		double amplitude = 1.0;
		double amplitudeSum = 0.0;
		double featureSize = p_featureSize;
		for (int octave = 0; octave < p_octaves; ++octave)
		{
			const std::uint64_t octaveSeed = deterministic::splitMix64Finalize(
				p_seed + static_cast<std::uint64_t>(octave) * 0x9e3779b97f4a7c15ULL);
			total += perlinNoise(octaveSeed, p_worldX, p_worldZ, featureSize) * amplitude;
			amplitudeSum += amplitude;
			amplitude *= p_persistence;
			featureSize *= 0.5;
		}
		return std::clamp(total / amplitudeSum, -1.0, 1.0);
	}

	PlanGrid<int> distanceTo(const Mask &p_mask)
	{
		const int size = p_mask.size();
		PlanGrid<int> distance(size, std::numeric_limits<int>::max() / 2);
		std::deque<Cell> queue;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (p_mask.at(i, j) != 0)
				{
					distance.at(i, j) = 0;
					queue.push_back({i, j});
				}
			}
		}
		while (!queue.empty())
		{
			const Cell cell = queue.front();
			queue.pop_front();
			forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
				if (distance.at(p_row, p_col) > distance.at(cell.row, cell.col) + 1)
				{
					distance.at(p_row, p_col) = distance.at(cell.row, cell.col) + 1;
					queue.push_back({p_row, p_col});
				}
			});
		}
		return distance;
	}

	std::vector<Cell> farthestPointSeeds(const std::vector<Cell> &p_candidates, int p_count, Rng &p_rng)
	{
		if (p_candidates.empty() || p_count <= 0)
		{
			return {};
		}
		const int total = static_cast<int>(p_candidates.size());
		std::vector<Cell> chosen;
		chosen.push_back(p_candidates[p_rng.below(total)]);
		std::vector<double> best(total);
		for (int index = 0; index < total; ++index)
		{
			const double dy = p_candidates[index].row - chosen[0].row;
			const double dx = p_candidates[index].col - chosen[0].col;
			best[index] = dy * dy + dx * dx;
		}
		while (static_cast<int>(chosen.size()) < std::min(p_count, total))
		{
			int farthest = 0;
			for (int index = 1; index < total; ++index)
			{
				if (best[index] > best[farthest])
				{
					farthest = index;
				}
			}
			chosen.push_back(p_candidates[farthest]);
			for (int index = 0; index < total; ++index)
			{
				const double dy = p_candidates[index].row - p_candidates[farthest].row;
				const double dx = p_candidates[index].col - p_candidates[farthest].col;
				best[index] = std::min(best[index], dy * dy + dx * dx);
			}
		}
		return chosen;
	}

	PlanGrid<int> labelComponents(const Mask &p_mask, int &p_count)
	{
		const int size = p_mask.size();
		PlanGrid<int> component(size, 0);
		p_count = 0;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (p_mask.at(i, j) == 0 || component.at(i, j) != 0)
				{
					continue;
				}
				++p_count;
				std::deque<Cell> queue{{i, j}};
				component.at(i, j) = p_count;
				while (!queue.empty())
				{
					const Cell cell = queue.front();
					queue.pop_front();
					forNeighbors4(cell.row, cell.col, size, [&](int p_row, int p_col) {
						if (p_mask.at(p_row, p_col) != 0 && component.at(p_row, p_col) == 0)
						{
							component.at(p_row, p_col) = p_count;
							queue.push_back({p_row, p_col});
						}
					});
				}
			}
		}
		return component;
	}

	int countDiagonalOnly(const Mask &p_mask)
	{
		const int size = p_mask.size();
		int bad = 0;
		for (int i = 0; i < size; ++i)
		{
			for (int j = 0; j < size; ++j)
			{
				if (p_mask.at(i, j) == 0)
				{
					continue;
				}
				bool orthogonal = false;
				forNeighbors4(i, j, size, [&](int p_row, int p_col) {
					orthogonal |= p_mask.at(p_row, p_col) != 0;
				});
				bool diagonal = false;
				for (const auto &[dr, dc] : {std::pair{-1, -1}, {-1, 1}, {1, -1}, {1, 1}})
				{
					const int row = i + dr;
					const int col = j + dc;
					if (row >= 0 && col >= 0 && row < size && col < size && p_mask.at(row, col) != 0)
					{
						diagonal = true;
					}
				}
				if (diagonal && !orthogonal)
				{
					++bad;
				}
			}
		}
		return bad;
	}
}
