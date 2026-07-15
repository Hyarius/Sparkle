#include "battle/presentation/battle_presentation_cell_source.hpp"

#include "board/cell_source.hpp"
#include "world/voxel_world.hpp"

#include "structures/voxel/spk_voxel_shape.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace
{
	[[nodiscard]] bool within(const pg::TraversalBounds &p_bounds, const spk::Vector3Int &p_cell) noexcept
	{
		return p_cell.x >= p_bounds.minimum.x && p_cell.x < p_bounds.maximum.x &&
			   p_cell.y >= p_bounds.minimum.y && p_cell.y < p_bounds.maximum.y &&
			   p_cell.z >= p_bounds.minimum.z && p_cell.z < p_bounds.maximum.z;
	}

	void include(spk::Vector3 &p_minimum, spk::Vector3 &p_maximum, const spk::Vector3 &p_value)
	{
		p_minimum.x = std::min(p_minimum.x, p_value.x);
		p_minimum.y = std::min(p_minimum.y, p_value.y);
		p_minimum.z = std::min(p_minimum.z, p_value.z);
		p_maximum.x = std::max(p_maximum.x, p_value.x);
		p_maximum.y = std::max(p_maximum.y, p_value.y);
		p_maximum.z = std::max(p_maximum.z, p_value.z);
	}

	template <typename TFace>
	void includeUpwardFace(
		spk::Vector3 &p_minimum,
		spk::Vector3 &p_maximum,
		bool &p_hasSurface,
		const TFace &p_face,
		const spk::Vector3 &p_offset)
	{
		for (const spk::VoxelShapePolygon &polygon : p_face.polygons())
		{
			if (polygon.size() < 3 || polygon.normal().y <= 0.0001f)
			{
				continue;
			}
			for (const spk::VoxelShapeVertex &vertex : polygon)
			{
				include(p_minimum, p_maximum, vertex.position + p_offset);
			}
			p_hasSurface = true;
		}
	}
}

namespace pg
{
	bool IBattlePresentationCellSource::isSolid(const spk::Vector3Int &p_presentationCell) const
	{
		const spk::VoxelCell *cell = tryCell(p_presentationCell);
		const VoxelDefinition *definition = cell == nullptr ? nullptr : tryDefinition(*cell);
		return definition != nullptr && definition->data.traversal == VoxelTraversal::Solid;
	}

	BoardPresentationCellSource::BoardPresentationCellSource(const BoardData &p_board) noexcept :
		_board(p_board)
	{
	}

	const BoardData &BoardPresentationCellSource::board() const noexcept
	{
		return _board;
	}

	bool BoardPresentationCellSource::_contains(const spk::Vector3Int &p_local) const noexcept
	{
		return within(_board.extent().traversalBounds, p_local);
	}

	const spk::VoxelCell *BoardPresentationCellSource::tryCell(const spk::Vector3Int &p_presentationCell) const
	{
		const spk::Vector3Int local = p_presentationCell - _board.presentationOrigin();
		if (!_contains(local))
		{
			return nullptr;
		}
		return &_board.cells().cell(local);
	}

	const VoxelDefinition *BoardPresentationCellSource::tryDefinition(const spk::VoxelCell &p_cell) const
	{
		return _board.cells().tryDefinition(p_cell);
	}

	const spk::VoxelShape *BoardPresentationCellSource::tryRenderShape(const spk::VoxelCell &p_cell) const
	{
		return _board.cells().tryRenderShape(p_cell);
	}

	VoxelWorldPresentationCellSource::VoxelWorldPresentationCellSource(const VoxelWorld &p_world) noexcept :
		_world(p_world)
	{
	}

	const spk::VoxelCell *VoxelWorldPresentationCellSource::tryCell(const spk::Vector3Int &p_presentationCell) const
	{
		return _world.tryCell(p_presentationCell);
	}

	const VoxelDefinition *VoxelWorldPresentationCellSource::tryDefinition(const spk::VoxelCell &p_cell) const
	{
		return _world.tryDefinition(p_cell);
	}

	const spk::VoxelShape *VoxelWorldPresentationCellSource::tryRenderShape(const spk::VoxelCell &p_cell) const
	{
		return p_cell.isEmpty() ? nullptr : &_world.registry().renderRegistry().shape(p_cell.id);
	}

	bool PresentationAabb::finite() const noexcept
	{
		return std::isfinite(minimum.x) && std::isfinite(minimum.y) && std::isfinite(minimum.z) &&
			   std::isfinite(maximum.x) && std::isfinite(maximum.y) && std::isfinite(maximum.z) &&
			   minimum.x <= maximum.x && minimum.y <= maximum.y && minimum.z <= maximum.z;
	}

	spk::Vector3 PresentationAabb::center() const noexcept
	{
		return (minimum + maximum) * 0.5f;
	}

	spk::Vector3 PresentationAabb::extent() const noexcept
	{
		return maximum - minimum;
	}

	PresentationAabb presentationBounds(const BoardData &p_board, const IBattlePresentationCellSource &p_cells)
	{
		const float infinity = std::numeric_limits<float>::infinity();
		spk::Vector3 minimum{infinity, infinity, infinity};
		spk::Vector3 maximum{-infinity, -infinity, -infinity};
		bool hasSurface = false;

		for (const TraversalGraph::Node &node : p_board.navigation().allNodes())
		{
			const spk::Vector3Int presentation = p_board.toPresentationCell(node.position);
			const spk::VoxelCell *cell = p_cells.tryCell(presentation);
			const spk::VoxelShape *shape = cell == nullptr ? nullptr : p_cells.tryRenderShape(*cell);
			if (shape == nullptr)
			{
				continue;
			}
			const spk::VoxelShapeFaceSet &faces = shape->renderFaces();
			const spk::Vector3 offset = spk::Vector3(presentation);
			for (std::size_t index = 0; index < faces.innerFaces.size(); ++index)
			{
				includeUpwardFace(minimum, maximum, hasSurface, shape->transformedInnerFace(index, cell->orientation, cell->flip), offset);
			}
			for (std::size_t index = 0; index < static_cast<std::size_t>(spk::VoxelAxisPlane::Count); ++index)
			{
				const auto plane = static_cast<spk::VoxelAxisPlane>(index);
				if (faces.outer(plane).has_value())
				{
					includeUpwardFace(minimum, maximum, hasSurface, shape->transformedOuterFace(plane, cell->orientation, cell->flip), offset);
				}
			}
		}
		PresentationAabb result{.minimum = minimum, .maximum = maximum};
		if (!hasSurface || !result.finite())
		{
			throw std::invalid_argument("battle presentation source has no finite walk surface");
		}
		return result;
	}
}
