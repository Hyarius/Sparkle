#pragma once

#include "board/board_navigation_layer.hpp"
#include "board/board_runtime_registry.hpp"
#include "board/board_terrain_layer.hpp"
#include "board/deployment.hpp"

#include <optional>
#include <vector>

namespace pg
{
	class BoardData
	{
	private:
		BoardTerrainLayer _terrain;
		BoardNavigationLayer _navigation;
		BoardRuntimeRegistry _runtime;
		spk::Vector3Int _worldAnchor{};
		std::vector<spk::Vector3Int> _borderCells;
		DeploymentZones _deploymentZones;

	public:
		BoardData(
			BoardTerrainLayer p_terrain,
			spk::Vector3Int p_worldAnchor = {},
			float p_maxVerticalGap = 0.5f);

		BoardData(BoardData &&) noexcept = default;
		BoardData &operator=(BoardData &&) noexcept = default;
		BoardData(const BoardData &) = delete;
		BoardData &operator=(const BoardData &) = delete;

		[[nodiscard]] const BoardTerrainLayer &terrain() const noexcept;
		[[nodiscard]] const BoardNavigationLayer &navigation() const noexcept;
		[[nodiscard]] BoardRuntimeRegistry &runtime() noexcept;
		[[nodiscard]] const BoardRuntimeRegistry &runtime() const noexcept;
		[[nodiscard]] const spk::Vector3Int &worldAnchor() const noexcept;
		[[nodiscard]] const std::vector<spk::Vector3Int> &borderCells() const noexcept;
		[[nodiscard]] const DeploymentZones &deploymentZones() const noexcept;

		void setDeploymentZones(DeploymentZones p_zones);
		[[nodiscard]] bool isInside(const spk::Vector3Int &p_local) const noexcept;
		[[nodiscard]] bool isStandable(const spk::Vector3Int &p_local) const noexcept;
		[[nodiscard]] bool isBorderCell(const spk::Vector3Int &p_local) const noexcept;
		[[nodiscard]] bool tryPlace(BattleObject &p_unit, const spk::Vector3Int &p_local);
		[[nodiscard]] bool tryMove(BattleObject &p_unit, const spk::Vector3Int &p_local);
		[[nodiscard]] bool swapUnits(BattleObject &p_first, BattleObject &p_second);
		[[nodiscard]] bool remove(BattleObject &p_unit) noexcept;
		[[nodiscard]] BattleObject *tryGetUnitAt(const spk::Vector3Int &p_local) const noexcept;
		[[nodiscard]] std::optional<spk::Vector3Int> tryGetPosition(const BattleObject &p_unit) const noexcept;

	private:
		void _computeBorderCells();
	};
}
