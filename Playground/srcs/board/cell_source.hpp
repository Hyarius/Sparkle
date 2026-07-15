#pragma once

#include "voxel/voxel_registry.hpp"
#include "voxel/voxel_traversal.hpp"

#include "structures/voxel/spk_voxel_grid.hpp"

#include <memory>

namespace pg
{
	class VoxelWorld;

	class ICellSource
	{
	public:
		virtual ~ICellSource() = default;
		[[nodiscard]] virtual const spk::VoxelCell &cell(const spk::Vector3Int &p_position) const = 0;
		// Type-level data (traversal, tags) of the cell's voxel, or nullptr when empty.
		[[nodiscard]] virtual const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const = 0;
		// State-level data (walk heights) of the cell's voxel state, or nullptr when empty.
		[[nodiscard]] virtual const VoxelStateDefinition *tryState(const spk::VoxelCell &p_cell) const = 0;
		// Rendering shape for this concrete voxel state.  The battle presentation is intentionally
		// built over the same frozen source as navigation, so it never needs to recover a VoxelWorld
		// from a live board or special-case an authored grid.
		[[nodiscard]] virtual const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const = 0;
	};

	// Reads an immutable voxel grid: a handcrafted arena materialised from a prefab, a synthetic
	// test fixture, or a short-lived navigation query over a grid the caller keeps alive.
	//
	// Two ownerships, deliberately: the owning constructor SHARES the grid, which is what a battle
	// board needs - the board outlives every construction temporary and must never point at a
	// stack-local grid - while the borrowing one keeps the existing navigation callers working. The
	// registry is always a borrow: Registries owns it and outlives every board built from it.
	class GridCellSource final : public ICellSource
	{
	private:
		std::shared_ptr<const spk::VoxelGrid> _ownedGrid;
		const spk::VoxelGrid &_grid;
		const VoxelRegistry &_registry;

	public:
		// Borrows the grid: it must outlive this source. BoardData never uses this constructor.
		GridCellSource(const spk::VoxelGrid &p_grid, const VoxelRegistry &p_registry);
		// Shares the grid, holding it for this source's whole lifetime. Throws std::invalid_argument
		// on a null grid.
		GridCellSource(std::shared_ptr<const spk::VoxelGrid> p_grid, const VoxelRegistry &p_registry);

		[[nodiscard]] const spk::VoxelGrid &grid() const noexcept;

		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const VoxelStateDefinition *tryState(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const override;
	};

	class WorldCellSource final : public ICellSource
	{
	private:
		const VoxelWorld &_world;
		spk::Vector3Int _originOffset{};

	public:
		explicit WorldCellSource(const VoxelWorld &p_world, spk::Vector3Int p_originOffset = {});
		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const VoxelStateDefinition *tryState(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const override;
	};

	[[nodiscard]] bool isSolid(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] bool isPassableSpace(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] bool isStandable(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] float walkHeightAtCenter(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] float walkHeightAtEdge(
		const ICellSource &p_source,
		const spk::Vector3Int &p_position,
		VoxelOrientation p_direction);
	[[nodiscard]] spk::Vector3 interpolateWalkSegment(
		const ICellSource &p_source,
		const spk::Vector3Int &p_from,
		const spk::Vector3Int &p_to,
		float p_progress);
}
