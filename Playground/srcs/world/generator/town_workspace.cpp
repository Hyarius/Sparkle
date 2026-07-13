#include "world/generator/town_workspace.hpp"

#include "world/generator/path_surface.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace pg
{
	std::size_t TownWorkspace::_index(TownColumn p_column) const
	{
		if (!contains(p_column))
			throw std::out_of_range("TownColumn is outside the town workspace");
		return static_cast<std::size_t>(p_column.z) * static_cast<std::size_t>(_width) + static_cast<std::size_t>(p_column.x);
	}

	TownWorkspace::TownWorkspace(const WorldPlan &p_plan, PlanTownSite p_site, TownComposition p_composition) :
		_plan(p_plan), _site(std::move(p_site)), _composition(std::move(p_composition))
	{
		if (_site.radiusColumns <= 0)
			throw std::invalid_argument("town workspace needs a positive radius");
		const int blocks = _plan.config.blocksPerCell;
		const std::int64_t centerX = static_cast<std::int64_t>(_plan.worldOffset()) + static_cast<std::int64_t>(_site.centerCol) * blocks + blocks / 2;
		const std::int64_t centerZ = static_cast<std::int64_t>(_plan.worldOffset()) + static_cast<std::int64_t>(_site.centerRow) * blocks + blocks / 2;
		const std::int64_t minimumX = centerX - _site.radiusColumns;
		const std::int64_t minimumZ = centerZ - _site.radiusColumns;
		const std::int64_t width = static_cast<std::int64_t>(_site.radiusColumns) * 2 + 1;
		if (minimumX < std::numeric_limits<int>::min() || minimumZ < std::numeric_limits<int>::min() ||
			minimumX + width - 1 > std::numeric_limits<int>::max() || minimumZ + width - 1 > std::numeric_limits<int>::max() ||
			width > 257)
			throw std::overflow_error("town workspace exceeds supported world-column bounds");
		_minimum = {static_cast<int>(minimumX), static_cast<int>(minimumZ)};
		_width = static_cast<int>(width);
		_layers.assign(static_cast<std::size_t>(_width) * static_cast<std::size_t>(_width), townLayer(TownWorkspaceLayer::Inside));
		_surfaceHeights.assign(_layers.size(), -1);
		for (int z = 0; z < _width; ++z)
		{
			for (int x = 0; x < _width; ++x)
			{
				const TownColumn local{x, z};
				const WorldColumn world = worldFromTown(local);
				const PlanCell cell = planCellFromWorld(world);
				std::uint32_t &layer = _layers[_index(local)];
				if (!_plan.land.contains(cell.row, cell.col) || _plan.land.at(cell.row, cell.col) == 0)
				{
					layer |= townLayer(TownWorkspaceLayer::Water) | townLayer(TownWorkspaceLayer::TerrainBlocked);
					continue;
				}
				if (_plan.water.at(cell.row, cell.col) != 0)
				{
					layer |= townLayer(TownWorkspaceLayer::Water) | townLayer(TownWorkspaceLayer::TerrainBlocked);
					continue;
				}
				layer |= townLayer(TownWorkspaceLayer::Land);
				_surfaceHeights[_index(local)] = _plan.surfaceY(_plan.height.at(cell.row, cell.col));
			}
		}
		// Earlier committed towns and structural infrastructure are hard obstacles
		// for this transaction.  Reservations may overlap at macro scale, but their
		// voxel-column claims never do.
		for (const PlanClaim &claim : _plan.townClaims)
		{
			const int minX = std::max(claim.min.x, _minimum.x);
			const int maxX = std::min(claim.max.x, _minimum.x + _width - 1);
			const int minZ = std::max(claim.min.z, _minimum.z);
			const int maxZ = std::min(claim.max.z, _minimum.z + _width - 1);
			for (int z = minZ; z <= maxZ; ++z)
				for (int x = minX; x <= maxX; ++x)
				{
					const TownColumn column = townFromWorld({x, z});
					const int surface = _surfaceHeights[_index(column)];
					if (surface >= claim.min.y && surface <= claim.max.y) _layers[_index(column)] |= townLayer(TownWorkspaceLayer::TerrainBlocked);
				}
		}
		_projectMainRoad();
	}

	TownWorldBounds TownWorkspace::bounds() const noexcept
	{
		return {.minX = _minimum.x, .maxX = _minimum.x + _width - 1, .minZ = _minimum.z, .maxZ = _minimum.z + _width - 1};
	}

	bool TownWorkspace::contains(TownColumn p_column) const noexcept
	{
		return p_column.x >= 0 && p_column.z >= 0 && p_column.x < _width && p_column.z < _width;
	}

	TownColumn TownWorkspace::townFromWorld(WorldColumn p_column) const
	{
		const TownColumn result{p_column.x - _minimum.x, p_column.z - _minimum.z};
		if (!contains(result))
			throw std::out_of_range("WorldColumn is outside the town workspace");
		return result;
	}

	WorldColumn TownWorkspace::worldFromTown(TownColumn p_column) const
	{
		if (!contains(p_column))
			throw std::out_of_range("TownColumn is outside the town workspace");
		return {_minimum.x + p_column.x, _minimum.z + p_column.z};
	}

	PlanCell TownWorkspace::planCellFromWorld(WorldColumn p_column) const
	{
		return {_plan.cellIndexFromWorld(p_column.z), _plan.cellIndexFromWorld(p_column.x)};
	}

	std::uint32_t TownWorkspace::layers(TownColumn p_column) const { return _layers[_index(p_column)]; }
	bool TownWorkspace::has(TownColumn p_column, TownWorkspaceLayer p_layer) const { return (layers(p_column) & townLayer(p_layer)) != 0; }
	void TownWorkspace::add(TownColumn p_column, TownWorkspaceLayer p_layer) { _layers[_index(p_column)] |= townLayer(p_layer); }
	void TownWorkspace::remove(TownColumn p_column, TownWorkspaceLayer p_layer) { _layers[_index(p_column)] &= ~townLayer(p_layer); }
	int TownWorkspace::surfaceHeight(TownColumn p_column) const { return _surfaceHeights[_index(p_column)]; }

	void TownWorkspace::_projectMainRoad()
	{
		const int blocks = _plan.config.blocksPerCell;
		const int offset = _plan.worldOffset();
		for (int z = 0; z < _width; ++z)
		{
			for (int x = 0; x < _width; ++x)
			{
				const TownColumn local{x, z};
				const WorldColumn world = worldFromTown(local);
				const PlanCell cell = planCellFromWorld(world);
				if (!_plan.road.contains(cell.row, cell.col) || _plan.road.at(cell.row, cell.col) == 0)
					continue;
				const int localX = world.x - (offset + cell.col * blocks);
				const int localZ = world.z - (offset + cell.row * blocks);
				const auto paved = [&](int p_row, int p_col) {
					return _plan.road.contains(p_row, p_col) && _plan.road.at(p_row, p_col) != 0;
				};
				if (isCenteredPathSurface(blocks, localX, localZ, paved(cell.row - 1, cell.col), paved(cell.row + 1, cell.col), paved(cell.row, cell.col - 1), paved(cell.row, cell.col + 1)))
					add(local, TownWorkspaceLayer::MainRoad);
			}
		}
	}

	std::vector<TownColumn> TownWorkspace::columnsWith(TownWorkspaceLayer p_layer) const
	{
		std::vector<TownColumn> result;
		for (int z = 0; z < _width; ++z)
			for (int x = 0; x < _width; ++x)
				if (has({x, z}, p_layer)) result.push_back({x, z});
		return result;
	}

	std::vector<PlanPavedColumn> TownWorkspace::mainRoadSurface() const
	{
		std::vector<PlanPavedColumn> result;
		for (TownColumn column : columnsWith(TownWorkspaceLayer::MainRoad))
		{
			const WorldColumn world = worldFromTown(column);
			result.push_back({.worldX = world.x, .worldZ = world.z, .surfaceY = surfaceHeight(column)});
		}
		std::sort(result.begin(), result.end());
		return result;
	}
}
