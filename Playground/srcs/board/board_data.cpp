#include "board/board_data.hpp"

#include <algorithm>

namespace pg
{
	BoardData::BoardData(
		BoardTerrainLayer p_terrain,
		spk::Vector3Int p_worldAnchor,
		float p_maxVerticalGap) :
		_terrain(std::move(p_terrain)),
		_navigation(_terrain, p_maxVerticalGap),
		_worldAnchor(p_worldAnchor)
	{
		_computeBorderCells();
	}

	const BoardTerrainLayer &BoardData::terrain() const noexcept
	{
		return _terrain;
	}
	const BoardNavigationLayer &BoardData::navigation() const noexcept
	{
		return _navigation;
	}
	BoardRuntimeRegistry &BoardData::runtime() noexcept
	{
		return _runtime;
	}
	const BoardRuntimeRegistry &BoardData::runtime() const noexcept
	{
		return _runtime;
	}
	const spk::Vector3Int &BoardData::worldAnchor() const noexcept
	{
		return _worldAnchor;
	}
	const std::vector<spk::Vector3Int> &BoardData::borderCells() const noexcept
	{
		return _borderCells;
	}
	const DeploymentZones &BoardData::deploymentZones() const noexcept
	{
		return _deploymentZones;
	}

	void BoardData::setDeploymentZones(DeploymentZones p_zones)
	{
		_deploymentZones = std::move(p_zones);
	}

	bool BoardData::isInside(const spk::Vector3Int &p_local) const noexcept
	{
		return _terrain.isInside(p_local);
	}
	bool BoardData::isStandable(const spk::Vector3Int &p_local) const noexcept
	{
		return _navigation.isStandable(p_local);
	}

	bool BoardData::isBorderCell(const spk::Vector3Int &p_local) const noexcept
	{
		return std::ranges::find(_borderCells, p_local) != _borderCells.end();
	}

	bool BoardData::tryPlace(BattleObject &p_unit, const spk::Vector3Int &p_local)
	{
		return isStandable(p_local) && _runtime.tryRegister(p_unit, p_local);
	}

	bool BoardData::tryMove(BattleObject &p_unit, const spk::Vector3Int &p_local)
	{
		return isStandable(p_local) && _runtime.tryMove(p_unit, p_local);
	}

	bool BoardData::swapUnits(BattleObject &p_first, BattleObject &p_second)
	{
		return _runtime.swapUnits(p_first, p_second);
	}

	bool BoardData::remove(BattleObject &p_unit) noexcept
	{
		return _runtime.remove(p_unit);
	}
	BattleObject *BoardData::tryGetUnitAt(const spk::Vector3Int &p_local) const noexcept
	{
		return _runtime.tryGetUnitAt(p_local);
	}

	std::optional<spk::Vector3Int> BoardData::tryGetPosition(const BattleObject &p_unit) const noexcept
	{
		return _runtime.tryGetPosition(p_unit);
	}

	void BoardData::_computeBorderCells()
	{
		const spk::Vector3Int size = _terrain.size();
		for (int z = 0; z < size.z; ++z)
		{
			for (int x = 0; x < size.x; ++x)
			{
				if (x != 0 && z != 0 && x != size.x - 1 && z != size.z - 1)
				{
					continue;
				}
				for (int y = size.y - 1; y >= 0; --y)
				{
					const spk::Vector3Int candidate{x, y, z};
					if (!isStandable(candidate))
					{
						continue;
					}
					_borderCells.push_back(candidate);
					break;
				}
			}
		}
	}
}
