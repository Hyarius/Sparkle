#include "structures/voxel/spk_voxel_chunk.hpp"

#include "structures/voxel/spk_prefab.hpp"
#include "structures/voxel/spk_voxel_map.hpp"

#include <stdexcept>
#include <thread>

namespace
{
	[[nodiscard]] constexpr int floorDivide(int p_value, int p_divisor) noexcept
	{
		const int quotient = p_value / p_divisor;
		const int remainder = p_value % p_divisor;
		return remainder < 0 ? quotient - 1 : quotient;
	}
}

namespace spk
{
	VoxelChunk::VoxelChunk(
		const spk::Vector3Int &p_coordinates,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		spk::Entity *p_parent) :
		VoxelChunk(p_coordinates, p_registry, p_worldLookup, nullptr, p_parent)
	{
	}

	VoxelChunk::VoxelChunk(
		const spk::Vector3Int &p_coordinates,
		const spk::VoxelRegistry &p_registry,
		const spk::IVoxelCellLookup *p_worldLookup,
		spk::VoxelMap *p_owner,
		spk::Entity *p_parent) :
		spk::Entity3D(p_parent),
		_coordinates(p_coordinates),
		_owner(p_owner)
	{
		transform().setPosition(spk::Vector3(worldOrigin()));
		_renderer = &addComponent<spk::VoxelChunkRenderer>(_grid, p_registry, p_worldLookup, worldOrigin());
	}

	VoxelChunk::Editor::Editor(spk::VoxelChunk &p_chunk) noexcept :
		_chunk(&p_chunk)
	{
	}

	const spk::Vector3Int &VoxelChunk::Editor::size() const noexcept
	{
		return _chunk->_grid.size();
	}

	bool VoxelChunk::Editor::isWithinBounds(const spk::Vector3Int &p_position) const noexcept
	{
		return _chunk->_grid.isWithinBounds(p_position);
	}

	bool VoxelChunk::Editor::isWithinBounds(int p_x, int p_y, int p_z) const noexcept
	{
		return isWithinBounds({p_x, p_y, p_z});
	}

	const spk::VoxelCell &VoxelChunk::Editor::cell(const spk::Vector3Int &p_position) const
	{
		return _chunk->_grid.cell(p_position);
	}

	const spk::VoxelCell &VoxelChunk::Editor::cell(int p_x, int p_y, int p_z) const
	{
		return cell({p_x, p_y, p_z});
	}

	bool VoxelChunk::Editor::setCell(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell)
	{
		_chunk->_assertMutationThread();
		spk::VoxelCell &current = _chunk->_grid.cell(p_position);
		if (current == p_cell)
		{
			return false;
		}

		current = p_cell;
		_changed = true;
		if (p_position.x == 0)
		{
			_changedBoundaries |= NegativeXBoundary;
		}
		if (p_position.x == Size.x - 1)
		{
			_changedBoundaries |= PositiveXBoundary;
		}
		if (p_position.y == 0)
		{
			_changedBoundaries |= NegativeYBoundary;
		}
		if (p_position.y == Size.y - 1)
		{
			_changedBoundaries |= PositiveYBoundary;
		}
		if (p_position.z == 0)
		{
			_changedBoundaries |= NegativeZBoundary;
		}
		if (p_position.z == Size.z - 1)
		{
			_changedBoundaries |= PositiveZBoundary;
		}
		return true;
	}

	bool VoxelChunk::Editor::setCell(int p_x, int p_y, int p_z, const spk::VoxelCell &p_cell)
	{
		return setCell({p_x, p_y, p_z}, p_cell);
	}

	void VoxelChunk::Editor::applyPrefab(
		const spk::Prefab &p_prefab,
		const spk::Vector3Int &p_destination,
		spk::VoxelOrientation p_orientation)
	{
		p_prefab.forEachAppliedVoxel(
			p_destination,
			p_orientation,
			[this](const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell) {
				if (isWithinBounds(p_position))
				{
					(void)setCell(p_position, p_cell);
				}
			});
	}

	void VoxelChunk::_assertMutationThread() const
	{
		if (_mutationThread.has_value() && *_mutationThread != std::this_thread::get_id())
		{
			throw std::logic_error("VoxelChunk cells may only be mutated on their owning update thread");
		}
	}

	void VoxelChunk::bindMutationThread(std::thread::id p_thread) noexcept
	{
		_mutationThread = p_thread;
	}

	void VoxelChunk::unbindMutationThread() noexcept
	{
		_mutationThread.reset();
	}

	bool VoxelChunk::hasBoundMutationThread() const noexcept
	{
		return _mutationThread.has_value();
	}

	void VoxelChunk::_commit(Editor &p_editor) noexcept
	{
		if (p_editor._changed == false)
		{
			return;
		}

		_renderer->requestSynchronization();
		++_contentRevision;
		if (_owner != nullptr)
		{
			_owner->_onChunkEdited(*this, p_editor._changedBoundaries);
		}
		p_editor._changed = false;
		p_editor._changedBoundaries = 0;
	}

	spk::Vector3Int VoxelChunk::coordinatesFromWorldCell(const spk::Vector3Int &p_worldCell) noexcept
	{
		return {
			floorDivide(p_worldCell.x, Size.x),
			floorDivide(p_worldCell.y, Size.y),
			floorDivide(p_worldCell.z, Size.z)};
	}

	const spk::Vector3Int &VoxelChunk::coordinates() const noexcept
	{
		return _coordinates;
	}

	spk::Vector3Int VoxelChunk::worldOrigin() const noexcept
	{
		return _coordinates * Size;
	}

	spk::Vector3Int VoxelChunk::localFromWorld(const spk::Vector3Int &p_worldCell) const noexcept
	{
		return p_worldCell - worldOrigin();
	}

	const spk::VoxelGrid &VoxelChunk::grid() const noexcept
	{
		return _grid;
	}

	std::uint64_t VoxelChunk::contentRevision() const noexcept
	{
		return _contentRevision;
	}

	const spk::VoxelCell &VoxelChunk::cell(const spk::Vector3Int &p_localPosition) const
	{
		return _grid.cell(p_localPosition);
	}

	void VoxelChunk::setCell(const spk::Vector3Int &p_localPosition, const spk::VoxelCell &p_cell)
	{
		editCells([&](Editor &p_editor) {
			(void)p_editor.setCell(p_localPosition, p_cell);
		});
	}

	spk::VoxelChunkRenderer &VoxelChunk::renderer() noexcept
	{
		return *_renderer;
	}

	const spk::VoxelChunkRenderer &VoxelChunk::renderer() const noexcept
	{
		return *_renderer;
	}
}
