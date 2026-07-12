#pragma once

#include "voxel/voxel_registry.hpp"
#include "voxel/voxel_traversal.hpp"

#include "structures/voxel/spk_voxel_grid.hpp"

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
	};

	class GridCellSource final : public ICellSource
	{
	private:
		const spk::VoxelGrid &_grid;
		const VoxelRegistry &_registry;

	public:
		GridCellSource(const spk::VoxelGrid &p_grid, const VoxelRegistry &p_registry);
		[[nodiscard]] const spk::VoxelCell &cell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const VoxelStateDefinition *tryState(const spk::VoxelCell &p_cell) const override;
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
