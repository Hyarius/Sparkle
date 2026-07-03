#pragma once

#include "structures/math/spk_vector3.hpp"

#include <vector>

namespace pg
{
	struct Ability;
	class BattleUnit;
	enum class BattleActionKind
	{
		Move,
		Ability,
		EndTurn
	};

	class BattleAction
	{
	public:
		BattleUnit &source;
		explicit BattleAction(BattleUnit &p_source) :
			source(p_source)
		{
		}
		virtual ~BattleAction() = default;
		[[nodiscard]] virtual BattleActionKind kind() const noexcept = 0;
		[[nodiscard]] virtual int apCost() const noexcept
		{
			return 0;
		}
		[[nodiscard]] virtual int mpCost() const noexcept
		{
			return 0;
		}
	};

	class MoveAction final : public BattleAction
	{
	public:
		spk::Vector3Int destination;
		int distance;
		MoveAction(BattleUnit &p_source, spk::Vector3Int p_destination, int p_distance) :
			BattleAction(p_source),
			destination(p_destination),
			distance(p_distance)
		{
		}
		[[nodiscard]] BattleActionKind kind() const noexcept override
		{
			return BattleActionKind::Move;
		}
		[[nodiscard]] int mpCost() const noexcept override
		{
			return distance;
		}
	};

	class AbilityAction final : public BattleAction
	{
	public:
		const Ability &ability;
		std::vector<spk::Vector3Int> targetCells;
		AbilityAction(BattleUnit &p_source, const Ability &p_ability, std::vector<spk::Vector3Int> p_targets) :
			BattleAction(p_source),
			ability(p_ability),
			targetCells(std::move(p_targets))
		{
		}
		[[nodiscard]] BattleActionKind kind() const noexcept override
		{
			return BattleActionKind::Ability;
		}
		[[nodiscard]] int apCost() const noexcept override;
		[[nodiscard]] int mpCost() const noexcept override;
	};

	class EndTurnAction final : public BattleAction
	{
	public:
		explicit EndTurnAction(BattleUnit &p_source) :
			BattleAction(p_source)
		{
		}
		[[nodiscard]] BattleActionKind kind() const noexcept override
		{
			return BattleActionKind::EndTurn;
		}
	};
}
