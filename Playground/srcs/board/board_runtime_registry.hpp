#pragma once

#include "battle/battle_object.hpp"
#include "board/traversal_graph.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class BoardRuntimeRegistry
	{
	private:
		struct ObjectEntry
		{
			BattleObject *object = nullptr;
			int remainingTurns = -1;
		};

		std::map<spk::Vector3Int, BattleObject *, CellPositionLess> _unitsByCell;
		std::map<const BattleObject *, spk::Vector3Int> _unitPositions;
		std::map<spk::Vector3Int, std::vector<ObjectEntry>, CellPositionLess> _objectsByCell;
		std::map<const BattleObject *, spk::Vector3Int> _objectPositions;

	public:
		[[nodiscard]] bool canRegister(const BattleObject &p_unit, const spk::Vector3Int &p_cell) const noexcept;
		[[nodiscard]] bool tryRegister(BattleObject &p_unit, const spk::Vector3Int &p_cell);
		[[nodiscard]] bool tryMove(BattleObject &p_unit, const spk::Vector3Int &p_to);
		[[nodiscard]] bool swapUnits(BattleObject &p_first, BattleObject &p_second);
		[[nodiscard]] bool remove(BattleObject &p_unit) noexcept;
		[[nodiscard]] std::optional<spk::Vector3Int> tryGetPosition(const BattleObject &p_unit) const noexcept;
		[[nodiscard]] BattleObject *tryGetUnitAt(const spk::Vector3Int &p_cell) const noexcept;
		[[nodiscard]] bool hasUnitAt(const spk::Vector3Int &p_cell) const noexcept;

		[[nodiscard]] bool tryAdd(BattleObject &p_object, const spk::Vector3Int &p_cell, int p_remainingTurns = -1);
		[[nodiscard]] std::vector<BattleObject *> getAt(const spk::Vector3Int &p_cell) const;
		[[nodiscard]] std::size_t removeByTags(
			const spk::Vector3Int &p_cell,
			const std::vector<std::string> &p_tags);
		[[nodiscard]] std::vector<BattleObject *> advanceObjectDurations();
	};
}
