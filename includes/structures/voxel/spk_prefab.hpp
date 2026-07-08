#pragma once

#include "structures/voxel/spk_voxel_grid.hpp"

#include <vector>

namespace spk
{
	// A reusable, rotatable batch of voxels (a "prefabricated structure") stamped into
	// voxel grids or maps. Only the listed voxels are applied: a voxel holding an empty
	// cell explicitly carves the target back to air, while positions that were never
	// listed leave the target untouched. Voxels are applied in insertion order.
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
		spk::Vector3Int _size{};
		std::vector<Voxel> _voxels;

	public:
		Prefab() = default;
		explicit Prefab(const spk::Vector3Int &p_size);
		// Lists every cell of the grid, empties included: applying the resulting prefab
		// overwrites its whole box, carving where the grid is empty.
		explicit Prefab(const spk::VoxelGrid &p_grid);

		void addVoxel(const spk::Vector3Int &p_position, const spk::VoxelCell &p_cell);
		// Adds p_cell to every position in the inclusive axis-aligned box. The two
		// corners may be provided in either order.
		void addVoxelRange(
			const spk::Vector3Int &p_firstPosition,
			const spk::Vector3Int &p_secondPosition,
			const spk::VoxelCell &p_cell);

		// The unrotated bounding box; voxels must lay within it.
		[[nodiscard]] const spk::Vector3Int &size() const noexcept;
		[[nodiscard]] const std::vector<Voxel> &voxels() const noexcept;

		// Size of the bounding box once rotated by the given orientation (quarter turns
		// around +Y; PositiveZ is the identity, matching VoxelOrientation semantics).
		[[nodiscard]] spk::Vector3Int rotatedSize(spk::VoxelOrientation p_orientation) const noexcept;

		// Stamps the prefab into the grid, the min corner of its rotated bounding box
		// landing on p_destination (grid coordinates, may reach outside the grid:
		// out-of-bounds voxels are skipped). Non-empty cells have their own orientation
		// rotated along with their position.
		void applyTo(
			spk::VoxelGrid &p_grid,
			const spk::Vector3Int &p_destination,
			spk::VoxelOrientation p_orientation = spk::VoxelOrientation::PositiveZ) const;
	};
}
