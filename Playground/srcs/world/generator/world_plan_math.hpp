#pragma once

#include "world/generator/world_plan.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

// Deterministic grid/noise toolbox of the world-plan generator. Randomness is drawn
// from generators seeded by hash(masterSeed, semanticPath) so adding a stage never
// reshuffles the output of existing stages.
namespace pg::worldgen
{
	struct Cell
	{
		int row = 0;
		int col = 0;

		[[nodiscard]] bool operator==(const Cell &) const noexcept = default;
	};

	// FNV-1a over "<master>::<path>"; stable across platforms and runs.
	[[nodiscard]] std::uint64_t deriveSeed(std::uint64_t p_master, const std::string &p_path);

	struct Rng
	{
		std::mt19937_64 engine;

		explicit Rng(std::uint64_t p_seed) :
			engine(p_seed)
		{
		}

		[[nodiscard]] double uniform()
		{
			return std::uniform_real_distribution<double>(0.0, 1.0)(engine);
		}
		[[nodiscard]] int below(int p_count)
		{
			return std::uniform_int_distribution<int>(0, p_count - 1)(engine);
		}
		[[nodiscard]] int poisson(double p_mean)
		{
			return p_mean <= 0.0 ? 0 : std::poisson_distribution<int>(p_mean)(engine);
		}
		template <typename TValue>
		void shuffle(std::vector<TValue> &p_values)
		{
			std::shuffle(p_values.begin(), p_values.end(), engine);
		}
	};

	using Mask = PlanGrid<std::uint8_t>;
	using Field = PlanGrid<double>;

	constexpr double INF = 1e18;

	// Neighbor order matches the Python _neighbors4 (north, south, west, east) so BFS
	// tie-breaks resolve the same way.
	template <typename TVisitor>
	void forNeighbors4(int p_row, int p_col, int p_size, TVisitor &&p_visit)
	{
		if (p_row > 0)
		{
			p_visit(p_row - 1, p_col);
		}
		if (p_row < p_size - 1)
		{
			p_visit(p_row + 1, p_col);
		}
		if (p_col > 0)
		{
			p_visit(p_row, p_col - 1);
		}
		if (p_col < p_size - 1)
		{
			p_visit(p_row, p_col + 1);
		}
	}

	// Cheap smooth value noise in [0,1]: bilinear over a random lattice with a
	// smoothstep on the fractional part (port of _value_noise).
	[[nodiscard]] Field valueNoise(Rng &p_rng, int p_size, double p_scale);

	// Multi-source BFS distance (orthogonal steps) to the True cells of the mask.
	[[nodiscard]] PlanGrid<int> distanceTo(const Mask &p_mask);

	// Farthest-point sampling: balanced spread of k picks over the candidate cells.
	[[nodiscard]] std::vector<Cell> farthestPointSeeds(const std::vector<Cell> &p_candidates, int p_count, Rng &p_rng);

	// Orthogonal connected components; 0 stays "not in mask", 1.. are component ids.
	[[nodiscard]] PlanGrid<int> labelComponents(const Mask &p_mask, int &p_count);

	// Mask cells whose only mask neighbors are diagonal (used by the MUST-be-0 stats).
	[[nodiscard]] int countDiagonalOnly(const Mask &p_mask);
}
