#include "battle/presentation/battle_board_picker.hpp"

#include "structures/voxel/spk_voxel_ray_cast.hpp"

#include <map>

namespace
{
	class PresentationLookup final : public spk::IVoxelCellLookup
	{
	private:
		const pg::IBattlePresentationCellSource &_source;

	public:
		explicit PresentationLookup(const pg::IBattlePresentationCellSource &p_source) :
			_source(p_source)
		{
		}

		[[nodiscard]] const spk::VoxelCell *tryCell(const spk::Vector3Int &p_cell) const override
		{
			return _source.tryCell(p_cell);
		}
	};
}

namespace pg
{
	std::optional<BattlePick> BattleBoardPicker::pick(
		const BoardData &p_board,
		const IBattlePresentationCellSource &p_presentationCells,
		const spk::Ray3D &p_ray,
		float p_maxDistance) const
	{
		// The frozen topology is the whitelist.  Building it at pick time keeps the picker a pure
		// value service and makes attach failures easy to test without a live world.
		std::map<spk::Vector3Int, BoardCell, CellPositionLess> whitelist;
		for (const TraversalGraph::Node &node : p_board.navigation().allNodes())
		{
			const spk::Vector3Int presentation = p_board.toPresentationCell(node.position);
			if (p_board.fromPresentationCell(presentation) != std::optional<BoardCell>(node.position) ||
				!whitelist.emplace(presentation, node.position).second)
			{
				return std::nullopt;
			}
		}

		const PresentationLookup lookup(p_presentationCells);
		const auto hit = spk::VoxelRayCast::cast(
			lookup,
			p_ray,
			p_maxDistance,
			[&p_presentationCells](const spk::Vector3Int &cell, const spk::VoxelCell &) {
				return p_presentationCells.isSolid(cell);
			});
		if (!hit.has_value())
		{
			return std::nullopt;
		}
		const auto iterator = whitelist.find(hit->cell);
		if (iterator == whitelist.end() || p_presentationCells.tryCell(hit->cell) == nullptr)
		{
			// The first solid voxel is either an obstacle, a non-board voxel, or terrain that no
			// longer exists in the source.  Do not search lower in the column or click through it.
			return std::nullopt;
		}
		return BattlePick{
			.boardCell = iterator->second,
			.presentationSupportCell = hit->cell,
			.cellEntryPosition = p_ray.origin + p_ray.direction * hit->distance,
			.rayDistance = hit->distance};
	}
}
