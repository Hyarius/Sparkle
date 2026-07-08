#pragma once

#include "structures/voxel/spk_voxel_grid.hpp"

#include <utility>
#include <vector>

namespace spk
{
	// A reusable, rotatable batch of voxels (a "prefabricated structure") stamped into
	// voxel grids or maps. Only the listed voxels are applied: a voxel holding an empty
	// cell explicitly carves the target back to air, while positions that were never
	// listed leave the target untouched. Voxels are applied in insertion order.
	//
	// Positions are trusted as authored: any coordinate may be negative, so a structure
	// can embed layers below its stamp destination (a floor slab, foundations) while
	// the destination itself stays at ground level. The bounding box is deduced from
	// the listed voxels, never declared.
	//
	// Stamping resolves each voxel as destination + rotation(position - pivot): the
	// pivot is the cell the prefab is "held by" — it lands exactly on the destination
	// whatever the orientation. It defaults to the local origin and can be set to any
	// position (it does not have to be the center).
	class Prefab
	{
	public:
		struct Voxel
		{
			spk::Vector3Int position;
			spk::VoxelCell cell;

			[[nodiscard]] bool operator==(const Voxel &) const noexcept = default;
		};

	private:
		spk::Vector3Int _pivot{};
		spk::Vector3Int _minBounds{};
		spk::Vector3Int _maxBounds{};
		std::vector<Voxel> _voxels;

		void _growBounds(const spk::Vector3Int &p_minimum, const spk::Vector3Int &p_maximum) noexcept;

	public:
		Prefab() = default;
		// Lists every cell of the grid, empties included, the grid's (0, 0, 0) corner
		// landing on p_origin: applying the resulting prefab overwrites its whole box,
		// carving where the grid is empty.
		explicit Prefab(const spk::VoxelGrid &p_grid, const spk::Vector3Int &p_origin = {0, 0, 0});

		void addVoxel(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell);
		// Adds p_cell to every position in the inclusive axis-aligned box. The two
		// corners may be provided in either order.
		void addVoxelRange(
			const spk::Vector3Int &p_firstPosition,
			const spk::Vector3Int &p_secondPosition,
			const spk::VoxelCell &p_cell);

		// The cell the prefab is held by while stamping and the fixed point of its
		// rotation; any position is valid, inside the voxels' box or not.
		void setPivot(const spk::Vector3Int &p_pivot) noexcept;
		[[nodiscard]] const spk::Vector3Int &pivot() const noexcept;

		// Bounding box of the listed voxels (inclusive corners), deduced as voxels are
		// added; both corners are zero while the prefab is empty.
		[[nodiscard]] const spk::Vector3Int &minBounds() const noexcept;
		[[nodiscard]] const spk::Vector3Int &maxBounds() const noexcept;
		// Extents of that box per axis (zero when the prefab is empty).
		[[nodiscard]] spk::Vector3Int size() const noexcept;
		[[nodiscard]] const std::vector<Voxel> &voxels() const noexcept;

		// Corners of the bounding box relative to the stamp destination once rotated by
		// the given orientation (quarter turns around +Y through the pivot; PositiveZ
		// is the identity, matching VoxelOrientation semantics). Zero for an empty
		// prefab.
		[[nodiscard]] std::pair<spk::Vector3Int, spk::Vector3Int> rotatedBounds(spk::VoxelOrientation p_orientation) const noexcept;

		// Stamps the prefab into the grid: every voxel lands at p_destination plus its
		// position rotated around the pivot, so p_destination is where the pivot lands
		// and negative-y layers land underneath it. Grid coordinates may reach outside
		// the grid: out-of-bounds voxels are skipped. Non-empty cells have their own
		// orientation rotated along with their position.
		void applyTo(
			spk::VoxelGrid &p_grid,
			const spk::Vector3Int &p_destination,
			spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ) const;
	};
}
