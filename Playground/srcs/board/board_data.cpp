#include "board/board_data.hpp"

#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_chunk.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace pg
{
	namespace
	{
		[[nodiscard]] std::string describeSource(const BoardSourceDescriptor &p_source)
		{
			const std::string kind = p_source.kind == BoardSourceKind::LiveWorld ? "liveWorld" : "handcrafted";
			return p_source.definitionId.has_value() ? kind + " '" + *p_source.definitionId + "'" : kind;
		}
	}

	std::string_view toString(BoardBuildErrorCode p_code) noexcept
	{
		switch (p_code)
		{
		case BoardBuildErrorCode::InvalidRequest:
			return "invalidRequest";
		case BoardBuildErrorCode::NumericOverflow:
			return "numericOverflow";
		case BoardBuildErrorCode::RequiredChunkUnavailable:
			return "requiredChunkUnavailable";
		case BoardBuildErrorCode::NoFeasibleWorldAnchor:
			return "noFeasibleWorldAnchor";
		case BoardBuildErrorCode::InvalidHandcraftedDefinition:
			return "invalidHandcraftedDefinition";
		case BoardBuildErrorCode::PrefabVoxelOutOfBounds:
			return "prefabVoxelOutOfBounds";
		case BoardBuildErrorCode::TraversalBuildFailed:
			return "traversalBuildFailed";
		case BoardBuildErrorCode::InsufficientDeploymentCapacity:
			return "insufficientDeploymentCapacity";
		}
		return "invalidRequest";
	}

	BoardData::BoardData(Parts p_parts) :
		_source(std::move(p_parts.source)),
		_cells(std::move(p_parts.cells)),
		_presentationOrigin(p_parts.presentationOrigin),
		_liveWorldAnchor(p_parts.liveWorldAnchor),
		_extent(p_parts.extent),
		_navigation(std::move(p_parts.navigation)),
		_occupancy(),
		_deployment(std::move(p_parts.deployment)),
		_borderCells(std::move(p_parts.borderCells)),
		_liveTerrainStamp(std::move(p_parts.liveTerrainStamp))
	{
		if (_cells == nullptr)
		{
			throw std::invalid_argument("a board owns a cell source");
		}
		if (_extent.size.x <= 0 || _extent.size.y <= 0)
		{
			throw std::invalid_argument("a board extent is strictly positive on both horizontal axes");
		}
		if (_extent.traversalBounds.minimum.x != 0 || _extent.traversalBounds.minimum.z != 0 ||
			_extent.traversalBounds.maximum.x != _extent.size.x ||
			_extent.traversalBounds.maximum.z != _extent.size.y ||
			_extent.traversalBounds.minimum.y >= _extent.traversalBounds.maximum.y)
		{
			throw std::invalid_argument("a board's traversal bounds must be its own [0, size) column box");
		}

		// The closed source invariants. A handcrafted arena may not masquerade as live terrain, and a
		// live board may not lose the anchor that gives its cells a world meaning.
		switch (_source.kind)
		{
		case BoardSourceKind::LiveWorld:
			if (_source.definitionId.has_value())
			{
				throw std::invalid_argument("a live-world board has no board definition id");
			}
			if (!_liveWorldAnchor.has_value() || !_liveTerrainStamp.has_value())
			{
				throw std::invalid_argument("a live-world board carries a world anchor and a terrain stamp");
			}
			if (_liveWorldAnchor->y != 0)
			{
				throw std::invalid_argument("a live world anchor never rebases Y: local Y is world Y");
			}
			if (_presentationOrigin != *_liveWorldAnchor)
			{
				throw std::invalid_argument("a live board is presented where it lives: its origin is its anchor");
			}
			if (_liveTerrainStamp->world == nullptr || _liveTerrainStamp->requiredChunks.empty())
			{
				throw std::invalid_argument("a live terrain stamp names its world and the chunks it read");
			}
			break;

		case BoardSourceKind::Handcrafted:
			if (!_source.definitionId.has_value() || _source.definitionId->empty())
			{
				throw std::invalid_argument("a handcrafted board carries the stable id it was built from");
			}
			if (_liveWorldAnchor.has_value() || _liveTerrainStamp.has_value())
			{
				throw std::invalid_argument("a handcrafted board has no world anchor and no terrain stamp");
			}
			if (_presentationOrigin != spk::Vector3Int{})
			{
				throw std::invalid_argument("a handcrafted board is presented at its own origin");
			}
			break;
		}

		_occupancy = BoardOccupancy(_navigation);
		std::ranges::sort(_borderCells, BoardCellLess{});
	}

	const BoardSourceDescriptor &BoardData::sourceDescriptor() const noexcept
	{
		return _source;
	}
	const ICellSource &BoardData::cells() const noexcept
	{
		return *_cells;
	}
	const TraversalGraph &BoardData::navigation() const noexcept
	{
		return _navigation;
	}
	const BoardOccupancy &BoardData::occupancy() const noexcept
	{
		return _occupancy;
	}
	const DeploymentLayout &BoardData::deployment() const noexcept
	{
		return _deployment;
	}
	const BoardExtent &BoardData::extent() const noexcept
	{
		return _extent;
	}
	const spk::Vector3Int &BoardData::presentationOrigin() const noexcept
	{
		return _presentationOrigin;
	}
	const std::optional<spk::Vector3Int> &BoardData::liveWorldAnchor() const noexcept
	{
		return _liveWorldAnchor;
	}
	const std::optional<LiveBoardTerrainStamp> &BoardData::liveTerrainStamp() const noexcept
	{
		return _liveTerrainStamp;
	}
	const std::vector<BoardCell> &BoardData::borderCells() const noexcept
	{
		return _borderCells;
	}

	bool BoardData::isInsideColumn(int p_x, int p_z) const noexcept
	{
		return p_x >= 0 && p_x < _extent.size.x && p_z >= 0 && p_z < _extent.size.y;
	}

	bool BoardData::isInside(const BoardCell &p_local) const noexcept
	{
		return isInsideColumn(p_local.x, p_local.z) && p_local.y >= _extent.traversalBounds.minimum.y &&
			   p_local.y < _extent.traversalBounds.maximum.y;
	}

	bool BoardData::isStandable(const BoardCell &p_local) const noexcept
	{
		return _navigation.tryGetNode(p_local) != nullptr;
	}

	bool BoardData::isBorderCell(const BoardCell &p_local) const noexcept
	{
		return std::ranges::binary_search(_borderCells, p_local, BoardCellLess{});
	}

	std::string BoardData::_describe(const BoardCell &p_local) const
	{
		std::string description = "board " + describeSource(_source) + ", local " + p_local.toString() +
								  ", presentation " + (tryAdd(p_local, _presentationOrigin).has_value() ? tryAdd(p_local, _presentationOrigin)->toString() : std::string("<overflow>"));
		if (_liveWorldAnchor.has_value())
		{
			const std::optional<spk::Vector3Int> world = tryAdd(p_local, *_liveWorldAnchor);
			description += ", world " + (world.has_value() ? world->toString() : std::string("<overflow>"));
		}
		return description;
	}

	int BoardData::movementCost(const BoardCell &p_local) const
	{
		requireCurrentTerrain();
		if (!isStandable(p_local))
		{
			throw std::runtime_error("movement cost asked of a cell no unit can stand on: " + _describe(p_local));
		}

		const spk::VoxelCell &support = _cells->cell(p_local);
		const VoxelDefinition *definition = _cells->tryDefinition(support);
		if (definition == nullptr)
		{
			throw std::runtime_error("a standable support voxel has no definition: " + _describe(p_local));
		}
		if (definition->data.movementCost <= 0)
		{
			throw std::runtime_error(
				"voxel '" + definition->id + "' has a non-positive movement cost: " + _describe(p_local));
		}
		return definition->data.movementCost;
	}

	spk::Vector3Int BoardData::toPresentationCell(const BoardCell &p_local) const
	{
		const std::optional<spk::Vector3Int> presentation = tryAdd(p_local, _presentationOrigin);
		if (!presentation.has_value())
		{
			throw std::overflow_error("presentation conversion overflowed: " + _describe(p_local));
		}
		return *presentation;
	}

	std::optional<BoardCell> BoardData::fromPresentationCell(const spk::Vector3Int &p_presentation) const
	{
		const std::optional<spk::Vector3Int> local = trySubtract(p_presentation, _presentationOrigin);
		if (!local.has_value() || !isInside(*local) || !isStandable(*local))
		{
			return std::nullopt;
		}
		return *local;
	}

	spk::Vector3Int BoardData::toLiveWorldCell(const BoardCell &p_local) const
	{
		if (!_liveWorldAnchor.has_value())
		{
			throw std::logic_error(
				"world coordinates asked of a board that does not live in the world: " + describeSource(_source));
		}
		const std::optional<spk::Vector3Int> world = tryAdd(p_local, *_liveWorldAnchor);
		if (!world.has_value())
		{
			throw std::overflow_error("world conversion overflowed: " + _describe(p_local));
		}
		return *world;
	}

	BoardCell BoardData::fromLiveWorldCell(const spk::Vector3Int &p_world) const
	{
		if (!_liveWorldAnchor.has_value())
		{
			throw std::logic_error(
				"world coordinates asked of a board that does not live in the world: " + describeSource(_source));
		}
		const std::optional<spk::Vector3Int> local = trySubtract(p_world, *_liveWorldAnchor);
		if (!local.has_value())
		{
			throw std::overflow_error("world conversion overflowed for world cell " + p_world.toString());
		}
		return *local;
	}

	spk::Vector3 BoardData::unitPresentationPosition(const BoardCell &p_local) const
	{
		const spk::Vector3Int presentation = toPresentationCell(p_local);
		return {
			static_cast<float>(presentation.x) + 0.5f,
			walkHeightAtCenter(*_cells, p_local),
			static_cast<float>(presentation.z) + 0.5f};
	}

	bool BoardData::terrainIsCurrent() const noexcept
	{
		if (!_liveTerrainStamp.has_value())
		{
			// The grid a handcrafted or synthetic board reads is owned and immutable, so nothing can
			// have moved under it.
			return true;
		}
		if (_liveTerrainStamp->mapLifetime.expired() || _liveTerrainStamp->world == nullptr)
		{
			return false;
		}

		const spk::VoxelMap &map = _liveTerrainStamp->world->map();
		for (const RequiredChunkStamp &stamped : _liveTerrainStamp->requiredChunks)
		{
			const spk::VoxelChunk *chunk = map.tryChunk(stamped.coordinates);
			if (chunk == nullptr || !chunk->isActivated() || chunk->contentRevision() != stamped.contentRevision)
			{
				return false;
			}
		}
		return true;
	}

	void BoardData::requireCurrentTerrain() const
	{
		if (!terrainIsCurrent())
		{
			throw std::runtime_error(
				"the terrain under board " + describeSource(_source) +
				" changed after it was built: the battle aborts rather than silently rebuilding its graph");
		}
	}

	TraversalCostQuery boardMovementQuery(
		const BoardData &p_board,
		BattleUnitId p_mover,
		const IBoardMovementBlockers *p_blockers)
	{
		return TraversalCostQuery{
			.enterCost = [&p_board, p_mover, p_blockers](const spk::Vector3Int &p_cell) -> std::optional<int> {
				const std::optional<BattleUnitId> occupant = p_board.occupancy().unitAt(p_cell);
				// The mover occupies its own source cell; every other unit - ally or enemy - blocks
				// both the traversal and the destination. There is no ally pass-through in v1.
				if (occupant.has_value() && *occupant != p_mover)
				{
					return std::nullopt;
				}
				if (p_blockers != nullptr && p_blockers->blocksMovement(p_cell))
				{
					return std::nullopt;
				}
				if (!p_board.isStandable(p_cell))
				{
					return std::nullopt;
				}
				return p_board.movementCost(p_cell);
			}};
	}

	BoardMutation::BoardMutation(BoardData &p_board) noexcept :
		_board(p_board)
	{
	}

	BoardMutationResult BoardMutation::placeUnit(BattleUnitId p_unit, BoardCell p_cell) const
	{
		return _board._occupancy.placeUnit(p_unit, p_cell);
	}

	BoardMutationResult BoardMutation::moveUnit(BattleUnitId p_unit, BoardCell p_destination) const
	{
		return _board._occupancy.moveUnit(p_unit, p_destination);
	}

	BoardMutationResult BoardMutation::swapUnits(BattleUnitId p_first, BattleUnitId p_second) const
	{
		return _board._occupancy.swapUnits(p_first, p_second);
	}

	bool BoardMutation::removeUnit(BattleUnitId p_unit) const noexcept
	{
		return _board._occupancy.removeUnit(p_unit);
	}

	BoardMutationResult BoardMutation::placeObject(BattleObjectId p_object, BoardCell p_cell) const
	{
		return _board._occupancy.placeObject(p_object, p_cell);
	}

	bool BoardMutation::removeObject(BattleObjectId p_object) const noexcept
	{
		return _board._occupancy.removeObject(p_object);
	}
}
