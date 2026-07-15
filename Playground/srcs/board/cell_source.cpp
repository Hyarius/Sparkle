#include "board/cell_source.hpp"

#include "world/voxel_world.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
	const spk::VoxelCell EmptyCell{};

	[[nodiscard]] const spk::VoxelGrid &requireGrid(const std::shared_ptr<const spk::VoxelGrid> &p_grid)
	{
		if (p_grid == nullptr)
		{
			throw std::invalid_argument("an owning cell source needs a grid to own");
		}
		return *p_grid;
	}
}

namespace pg
{
	GridCellSource::GridCellSource(const spk::VoxelGrid &p_grid, const VoxelRegistry &p_registry) :
		_ownedGrid(nullptr),
		_grid(p_grid),
		_registry(p_registry)
	{
	}

	GridCellSource::GridCellSource(std::shared_ptr<const spk::VoxelGrid> p_grid, const VoxelRegistry &p_registry) :
		_ownedGrid(std::move(p_grid)),
		// _ownedGrid is declared first, so it is already initialised: the reference binds to the grid
		// this source now co-owns, and the null case throws before anything is dereferenced.
		_grid(requireGrid(_ownedGrid)),
		_registry(p_registry)
	{
	}

	const spk::VoxelGrid &GridCellSource::grid() const noexcept
	{
		return _grid;
	}

	const spk::VoxelCell &GridCellSource::cell(const spk::Vector3Int &p_position) const
	{
		return _grid.isWithinBounds(p_position) ? _grid.cell(p_position) : EmptyCell;
	}

	const VoxelDefinition *GridCellSource::tryDefinition(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry.tryDefinition(p_cell.id);
	}

	const VoxelStateDefinition *GridCellSource::tryState(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _registry.tryState(p_cell.id);
	}

	WorldCellSource::WorldCellSource(const VoxelWorld &p_world, spk::Vector3Int p_originOffset) :
		_world(p_world),
		_originOffset(p_originOffset)
	{
	}

	const spk::VoxelCell &WorldCellSource::cell(const spk::Vector3Int &p_position) const
	{
		return _world.cell(p_position + _originOffset);
	}

	const VoxelDefinition *WorldCellSource::tryDefinition(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _world.registry().tryDefinition(p_cell.id);
	}

	const VoxelStateDefinition *WorldCellSource::tryState(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : _world.registry().tryState(p_cell.id);
	}

	bool isSolid(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		const spk::VoxelCell &value = p_source.cell(p_position);
		const VoxelDefinition *definition = p_source.tryDefinition(value);
		return definition != nullptr && definition->data.traversal == VoxelTraversal::Solid;
	}

	bool isPassableSpace(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		const spk::VoxelCell &value = p_source.cell(p_position);
		if (value.isEmpty())
		{
			return true;
		}
		const VoxelDefinition *definition = p_source.tryDefinition(value);
		return definition != nullptr && definition->data.traversal == VoxelTraversal::Passable;
	}

	bool isStandable(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		return isSolid(p_source, p_position) &&
			   isPassableSpace(p_source, p_position + spk::Vector3Int{0, 1, 0}) &&
			   isPassableSpace(p_source, p_position + spk::Vector3Int{0, 2, 0});
	}

	float walkHeightAtCenter(const ICellSource &p_source, const spk::Vector3Int &p_position)
	{
		const spk::VoxelCell &cell = p_source.cell(p_position);
		const VoxelStateDefinition *state = p_source.tryState(cell);
		if (state == nullptr)
		{
			return static_cast<float>(p_position.y);
		}
		return static_cast<float>(p_position.y) +
			   resolveWorldHeights(state->heights.get(cell.flip), cell.orientation).stationary;
	}

	float walkHeightAtEdge(
		const ICellSource &p_source,
		const spk::Vector3Int &p_position,
		VoxelOrientation p_direction)
	{
		const spk::VoxelCell &cell = p_source.cell(p_position);
		const VoxelStateDefinition *state = p_source.tryState(cell);
		if (state == nullptr)
		{
			return static_cast<float>(p_position.y);
		}
		return static_cast<float>(p_position.y) +
			   resolveWorldHeights(state->heights.get(cell.flip), cell.orientation).get(p_direction);
	}

	spk::Vector3 interpolateWalkSegment(
		const ICellSource &p_source,
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to,
		float p_progress)
	{
		const float progress = std::clamp(p_progress, 0.0f, 1.0f);
		VoxelOrientation direction = VoxelOrientation::PositiveZ;
		VoxelOrientation opposite = VoxelOrientation::NegativeZ;
		if (p_to.x > p_from.x)
		{
			direction = VoxelOrientation::PositiveX;
			opposite = VoxelOrientation::NegativeX;
		}
		else if (p_to.x < p_from.x)
		{
			direction = VoxelOrientation::NegativeX;
			opposite = VoxelOrientation::PositiveX;
		}
		else if (p_to.z < p_from.z)
		{
			direction = VoxelOrientation::NegativeZ;
			opposite = VoxelOrientation::PositiveZ;
		}

		const float fromHeight = walkHeightAtCenter(p_source, p_from);
		const float toHeight = walkHeightAtCenter(p_source, p_to);
		const float boundary = 0.5f * (walkHeightAtEdge(p_source, p_from, direction) +
									   walkHeightAtEdge(p_source, p_to, opposite));
		const float height = progress <= 0.5f
								 ? fromHeight + (boundary - fromHeight) * (progress * 2.0f)
								 : boundary + (toHeight - boundary) * ((progress - 0.5f) * 2.0f);
		return {
			static_cast<float>(p_from.x) + 0.5f + static_cast<float>(p_to.x - p_from.x) * progress,
			height,
			static_cast<float>(p_from.z) + 0.5f + static_cast<float>(p_to.z - p_from.z) * progress};
	}
}
