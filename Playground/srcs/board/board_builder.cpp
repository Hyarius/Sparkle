#include "board/board_builder.hpp"

#include "board/cell_source.hpp"
#include "board/deployment.hpp"
#include "voxel/voxel_grid.hpp"
#include "world/chunk.hpp"
#include "world/voxel_world.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace
{
	struct WorldBounds
	{
		spk::Vector3Int minimum{};
		spk::Vector3Int maximum{};
	};

	[[nodiscard]] WorldBounds loadedBounds(const pg::VoxelWorld &p_world)
	{
		const std::vector<pg::ChunkCoordinates> chunks = p_world.loadedChunkCoordinates();
		if (chunks.empty())
		{
			throw std::invalid_argument("Cannot derive a board from an empty world");
		}
		WorldBounds result{
			.minimum = spk::Vector3Int(std::numeric_limits<int>::max()),
			.maximum = spk::Vector3Int(std::numeric_limits<int>::lowest())};
		for (const pg::ChunkCoordinates &chunk : chunks)
		{
			const spk::Vector3Int origin = chunk.worldOrigin();
			result.minimum = spk::Vector3Int::min(result.minimum, origin);
			result.maximum = spk::Vector3Int::max(result.maximum, origin + pg::Chunk::Size);
		}
		return result;
	}

	[[nodiscard]] spk::Vector3Int centeredAnchor(
		const spk::Vector3Int &p_center,
		const spk::Vector2Int &p_size,
		const WorldBounds &p_bounds)
	{
		return {
			std::clamp(p_center.x - p_size.x / 2, p_bounds.minimum.x, p_bounds.maximum.x - p_size.x),
			p_bounds.minimum.y,
			std::clamp(p_center.z - p_size.y / 2, p_bounds.minimum.z, p_bounds.maximum.z - p_size.y)};
	}

	[[nodiscard]] spk::Vector3Int nudgedAnchor(
		const spk::Vector3Int &p_anchor,
		const spk::Vector3Int &p_nudge,
		const spk::Vector2Int &p_size,
		const WorldBounds &p_bounds)
	{
		return {
			std::clamp(p_anchor.x + p_nudge.x, p_bounds.minimum.x, p_bounds.maximum.x - p_size.x),
			p_bounds.minimum.y,
			std::clamp(p_anchor.z + p_nudge.z, p_bounds.minimum.z, p_bounds.maximum.z - p_size.y)};
	}

	[[nodiscard]] pg::BoardData buildWorldBoard(
		const pg::VoxelWorld &p_world,
		const WorldBounds &p_bounds,
		const spk::Vector3Int &p_anchor,
		const spk::Vector2Int &p_size,
		int p_deploymentDepth,
		pg::VoxelOrientation p_playerFacing,
		float p_maxVerticalGap)
	{
		const spk::Vector3Int terrainSize{
			p_size.x,
			p_bounds.maximum.y - p_bounds.minimum.y,
			p_size.y};
		pg::BoardData board(
			pg::BoardTerrainLayer(
				std::make_unique<pg::WorldCellSource>(p_world, p_anchor),
				terrainSize),
			p_anchor,
			p_maxVerticalGap);
		board.setDeploymentZones(pg::Deployment::compute(board, p_deploymentDepth, p_playerFacing));
		return board;
	}

	[[nodiscard]] bool isValid(const pg::BoardData &p_board, std::size_t p_teamSize)
	{
		return p_board.navigation().graph().size() >= p_teamSize * 4 &&
			   p_board.deploymentZones().player.size() >= p_teamSize &&
			   p_board.deploymentZones().enemy.size() >= p_teamSize;
	}

	[[nodiscard]] double candidateScore(const pg::BoardData &p_board)
	{
		const std::size_t deploymentCapacity = std::min(
			p_board.deploymentZones().player.size(),
			p_board.deploymentZones().enemy.size());
		const spk::Vector3Int size = p_board.terrain().size();
		const double columnCount = static_cast<double>(size.x) * static_cast<double>(size.z);
		const double density = columnCount <= 0.0
								   ? 0.0
								   : static_cast<double>(p_board.navigation().graph().size()) / columnCount;
		return static_cast<double>(deploymentCapacity) * 100000.0 + density * 1000.0 +
			   static_cast<double>(p_board.navigation().graph().size());
	}
}

namespace pg
{
	BoardData BoardBuilder::fromWorld(
		const VoxelWorld &p_world,
		const spk::Vector3Int &p_center,
		const spk::Vector2Int &p_size,
		int p_deploymentStripDepth,
		std::size_t p_teamSize,
		VoxelOrientation p_playerFacing,
		float p_maxVerticalGap)
	{
		if (p_size.x <= 0 || p_size.y <= 0)
		{
			throw std::invalid_argument("Board dimensions must be positive");
		}
		if (p_deploymentStripDepth <= 0)
		{
			throw std::invalid_argument("Deployment strip depth must be positive");
		}
		if (p_teamSize == 0)
		{
			throw std::invalid_argument("Team size must be positive");
		}
		const WorldBounds bounds = loadedBounds(p_world);
		const spk::Vector2Int loadedSize{bounds.maximum.x - bounds.minimum.x, bounds.maximum.z - bounds.minimum.z};
		const spk::Vector2Int initialSize{
			std::min(p_size.x, loadedSize.x),
			std::min(p_size.y, loadedSize.y)};
		const std::array nudges = {
			spk::Vector3Int{0, 0, 0},
			spk::Vector3Int{2, 0, 0},
			spk::Vector3Int{-2, 0, 0},
			spk::Vector3Int{0, 0, 2},
			spk::Vector3Int{0, 0, -2}};

		spk::Vector2Int finalSize = initialSize;
		spk::Vector3Int finalAnchor = centeredAnchor(p_center, initialSize, bounds);
		double bestScore = -1.0;
		for (int shrinkAttempt = 0; shrinkAttempt < 4; ++shrinkAttempt)
		{
			const spk::Vector2Int candidateSize{
				std::max(1, initialSize.x - shrinkAttempt * 2),
				std::max(1, initialSize.y - shrinkAttempt * 2)};
			const spk::Vector3Int baseAnchor = centeredAnchor(p_center, candidateSize, bounds);
			for (const spk::Vector3Int &nudge : nudges)
			{
				const spk::Vector3Int anchor = nudgedAnchor(baseAnchor, nudge, candidateSize, bounds);
				BoardData candidate = buildWorldBoard(
					p_world, bounds, anchor, candidateSize, p_deploymentStripDepth, p_playerFacing, p_maxVerticalGap);
				const double score = candidateScore(candidate);
				if (score > bestScore)
				{
					bestScore = score;
					finalSize = candidateSize;
					finalAnchor = anchor;
				}
				if (isValid(candidate, p_teamSize))
				{
					if (candidateSize != initialSize)
					{
						std::cerr << "BoardBuilder: warning: shrank board from " << initialSize.x << 'x' << initialSize.y
								  << " to " << candidateSize.x << 'x' << candidateSize.y << std::endl;
					}
					return candidate;
				}
			}
		}
		if (finalSize != initialSize)
		{
			std::cerr << "BoardBuilder: warning: pathological terrain forced board shrink from "
					  << initialSize.x << 'x' << initialSize.y << " to " << finalSize.x << 'x' << finalSize.y
					  << std::endl;
		}
		return buildWorldBoard(
			p_world, bounds, finalAnchor, finalSize, p_deploymentStripDepth, p_playerFacing, p_maxVerticalGap);
	}

	BoardData BoardBuilder::fromGrid(
		const VoxelGrid &p_grid,
		const VoxelRegistry &p_registry,
		int p_deploymentStripDepth,
		VoxelOrientation p_playerFacing,
		float p_maxVerticalGap)
	{
		if (p_deploymentStripDepth <= 0)
		{
			throw std::invalid_argument("Deployment strip depth must be positive");
		}
		BoardData board(BoardTerrainLayer(std::make_unique<GridCellSource>(p_grid, p_registry), p_grid.size()), {}, p_maxVerticalGap);
		board.setDeploymentZones(Deployment::compute(board, p_deploymentStripDepth, p_playerFacing));
		return board;
	}
}
