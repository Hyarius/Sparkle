#include "battle/battle_pump.hpp"

#include "battle/battle_session.hpp"

#include <variant>

namespace pg
{
	BattlePumpResult BattlePump::advanceUntilPlayerInput(BattleSession &p_session, std::size_t p_maxLogicalOperations)
	{
		BattlePumpResult result;
		while (true)
		{
			if (p_session.phase() == BattlePhase::Terminal)
			{
				result.state = BattlePumpState::Terminal;
				return result;
			}
			if (p_session.phase() == BattlePhase::Deployment)
			{
				result.state = BattlePumpState::NeedsDeployment;
				return result;
			}
			if (result.logicalOperations >= p_maxLogicalOperations)
			{
				const BattleSnapshot snapshot = p_session.snapshot();
				if (snapshot.phase == BattlePhase::Activation && snapshot.activeUnit.has_value())
				{
					const auto unit = std::ranges::find(snapshot.units, *snapshot.activeUnit, &BattleUnitSnapshot::id);
					if (unit != snapshot.units.end() &&
						(unit->side == BattleSide::Enemy || (unit->side == BattleSide::Player && _debugPlayerAutoplayBehaviour.has_value())))
					{
						result.state = BattlePumpState::WaitingForAI;
						return result;
					}
				}
				result.state = BattlePumpState::Progressed;
				return result;
			}

			if (p_session.phase() == BattlePhase::AwaitingActivation)
			{
				const SchedulerCallResult scheduled = p_session.advanceUntilActivation();
				++result.logicalOperations;
				if (std::holds_alternative<RejectedSchedulerAdvance>(scheduled))
				{
					result.state = BattlePumpState::Progressed;
					return result;
				}
				continue;
			}

			const BattleSnapshot snapshot = p_session.snapshot();
			const auto active = snapshot.activeUnit.has_value()
									? std::ranges::find(snapshot.units, *snapshot.activeUnit, &BattleUnitSnapshot::id)
									: snapshot.units.end();
			if (active == snapshot.units.end())
			{
				result.state = BattlePumpState::Progressed;
				return result;
			}
			if (active->side == BattleSide::Player && !_debugPlayerAutoplayBehaviour.has_value())
			{
				result.state = BattlePumpState::WaitingForPlayer;
				return result;
			}

			const CommandIssuer issuer{active->side == BattleSide::Enemy ? CommandController::EnemyAi : CommandController::DebugAutoplay};
			const std::optional<std::string_view> override = active->side == BattleSide::Player
																 ? std::optional<std::string_view>(*_debugPlayerAutoplayBehaviour)
																 : std::nullopt;
			AIDriverResult driven = _driver.executeOne(p_session, issuer, override);
			++result.logicalOperations;
			result.diagnostics.insert(
				result.diagnostics.end(),
				std::make_move_iterator(driven.diagnostics.begin()),
				std::make_move_iterator(driven.diagnostics.end()));
			if (driven.operation == AIDriverOperation::Terminal)
			{
				result.state = BattlePumpState::Terminal;
				return result;
			}
			if (driven.operation == AIDriverOperation::NoOperation)
			{
				result.state = BattlePumpState::Progressed;
				return result;
			}
		}
	}
}
