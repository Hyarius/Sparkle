#pragma once

#include "board/board_data.hpp"

#include "structures/math/spk_vector3.hpp"
#include "structures/voxel/spk_voxel_cell.hpp"

namespace spk
{
	class VoxelShape;
}

namespace pg
{
	class VoxelDefinition;
	class VoxelWorld;

	// Presentation coordinates are the coordinates used by the persistent GameSceneWidget.  A
	// live board maps them into the frozen world window; an authored arena maps them into its
	// immutable zero-origin grid.  This is deliberately not a navigation or LoS interface.
	class IBattlePresentationCellSource
	{
	public:
		virtual ~IBattlePresentationCellSource() = default;

		[[nodiscard]] virtual const spk::VoxelCell *tryCell(const spk::Vector3Int &p_presentationCell) const = 0;
		[[nodiscard]] virtual const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const = 0;
		[[nodiscard]] virtual const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const = 0;

		[[nodiscard]] bool isSolid(const spk::Vector3Int &p_presentationCell) const;
	};

	// A source-agnostic adapter over the BoardData source already frozen by step 12.  It keeps the
	// presentation transform in one place and lets both live and handcrafted boards use identical
	// mask extraction and ray traversal code.
	class BoardPresentationCellSource final : public IBattlePresentationCellSource
	{
	private:
		const BoardData &_board;

		[[nodiscard]] bool _contains(const spk::Vector3Int &p_local) const noexcept;

	public:
		explicit BoardPresentationCellSource(const BoardData &p_board) noexcept;

		[[nodiscard]] const BoardData &board() const noexcept;
		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_presentationCell) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const override;
	};

	// The exploration hover uses exactly the same source contract as battle, preventing the two
	// paths from gradually disagreeing about slopes, stairs, atlas UVs, or mask lift.
	class VoxelWorldPresentationCellSource final : public IBattlePresentationCellSource
	{
	private:
		const VoxelWorld &_world;

	public:
		explicit VoxelWorldPresentationCellSource(const VoxelWorld &p_world) noexcept;

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_presentationCell) const override;
		[[nodiscard]] const VoxelDefinition *tryDefinition(const spk::VoxelCell &p_cell) const override;
		[[nodiscard]] const spk::VoxelShape *tryRenderShape(const spk::VoxelCell &p_cell) const override;
	};

	struct PresentationAabb
	{
		spk::Vector3 minimum{};
		spk::Vector3 maximum{};

		[[nodiscard]] bool finite() const noexcept;
		[[nodiscard]] spk::Vector3 center() const noexcept;
		[[nodiscard]] spk::Vector3 extent() const noexcept;
	};

	struct BattlePresentationBoardBinding
	{
		const BoardData &board;
		const IBattlePresentationCellSource &cells;
		PresentationAabb bounds;
		BoardSourceDescriptor source;
	};

	// Scans the exact upward support surfaces present in the frozen board.  An empty/non-finite
	// result is an attachment failure: input should never enter a battle it cannot present.
	[[nodiscard]] PresentationAabb presentationBounds(
		const BoardData &p_board,
		const IBattlePresentationCellSource &p_cells);
}
