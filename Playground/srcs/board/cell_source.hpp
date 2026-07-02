#pragma once

#include "voxel/voxel_grid.hpp"
#include "voxel/voxel_registry.hpp"

namespace pg
{
	class VoxelWorld;

	class ICellSource
	{
	public:
		virtual ~ICellSource() = default;
		[[nodiscard]] virtual const VoxelCell &cell(const spk::Vector3Int &p_position) const = 0;
		[[nodiscard]] virtual const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const = 0;
	};

	class GridCellSource final : public ICellSource
	{
	private:
		const VoxelGrid &_grid;
		const VoxelRegistry &_registry;

	public:
		GridCellSource(const VoxelGrid &p_grid, const VoxelRegistry &p_registry);
		[[nodiscard]] const VoxelCell &cell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const override;
	};

	class WorldCellSource final : public ICellSource
	{
	private:
		const VoxelWorld &_world;

	public:
		explicit WorldCellSource(const VoxelWorld &p_world);
		[[nodiscard]] const VoxelCell &cell(const spk::Vector3Int &p_position) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const VoxelCell &p_cell) const override;
	};

	[[nodiscard]] bool isSolid(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] bool isPassableSpace(const ICellSource &p_source, const spk::Vector3Int &p_position);
	[[nodiscard]] bool isStandable(const ICellSource &p_source, const spk::Vector3Int &p_position);
}
