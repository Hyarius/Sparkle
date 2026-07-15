#include "battle/ai/battle_ai_driver.hpp"

#include "battle/battle_session.hpp"
#include "core/deterministic_random.hpp"
#include "core/registries.hpp"

#include <algorithm>
#include <type_traits>

namespace pg
{
	namespace
	{
		using spk::deterministic::StableHasher64;

		[[nodiscard]] bool isNonEnd(const BattleCommand &p_command) noexcept
		{
			return std::holds_alternative<MoveCommand>(p_command) || std::holds_alternative<CastAbilityCommand>(p_command);
		}

		[[nodiscard]] const BattleUnitSnapshot *activeSnapshotUnit(const BattleSnapshot &p_snapshot) noexcept
		{
			if (!p_snapshot.activeUnit.has_value())
			{
				return nullptr;
			}
			const auto found = std::ranges::find(p_snapshot.units, *p_snapshot.activeUnit, &BattleUnitSnapshot::id);
			return found == p_snapshot.units.end() ? nullptr : &*found;
		}

		void mixU64(StableHasher64 &p_hasher, std::uint64_t p_value) noexcept
		{
			p_hasher.mix(static_cast<std::int32_t>(p_value & 0xFFFFFFFFU));
			p_hasher.mix(static_cast<std::int32_t>(p_value >> 32U));
		}
	}

	void BattleAIDriver::_clearIfActivationChanged(const BattleSession &p_session)
	{
		if (!_guard.has_value())
		{
			return;
		}
		const BattleSnapshot snapshot = p_session.snapshot();
		if (snapshot.phase != BattlePhase::Activation || !snapshot.turn.has_value() || !snapshot.activeUnit.has_value() ||
			*snapshot.turn != _guard->turn || *snapshot.activeUnit != _guard->unit)
		{
			_guard.reset();
		}
	}

	AIActivationGuardState &BattleAIDriver::_guardFor(const BattleSession &p_session)
	{
		const BattleSnapshot snapshot = p_session.snapshot();
		if (!_guard.has_value() || snapshot.phase != BattlePhase::Activation || !snapshot.turn.has_value() ||
			!snapshot.activeUnit.has_value() || _guard->turn != *snapshot.turn || _guard->unit != *snapshot.activeUnit)
		{
			_guard = AIActivationGuardState{*snapshot.turn, *snapshot.activeUnit};
		}
		return *_guard;
	}

	AIDriverResult BattleAIDriver::_submitEnd(
		BattleSession &p_session,
		BattleUnitId p_unit,
		CommandIssuer p_issuer,
		EndTurnRequestCause p_cause,
		std::vector<AICommandDiagnostic> p_diagnostics)
	{
		const CommandResult result = p_session.submit(EndTurnCommand{p_unit, p_cause}, p_issuer);
		_clearIfActivationChanged(p_session);
		return AIDriverResult{
			.operation = std::holds_alternative<AcceptedCommand>(result) ? AIDriverOperation::SafelyEnded : (std::holds_alternative<AbortedCommand>(result) ? AIDriverOperation::Terminal : AIDriverOperation::NoOperation),
			.commandResult = result,
			.diagnostics = std::move(p_diagnostics)};
	}

	AIDriverResult BattleAIDriver::executeOne(
		BattleSession &p_session,
		CommandIssuer p_issuer,
		std::optional<std::string_view> p_behaviourOverride)
	{
		_clearIfActivationChanged(p_session);
		const BattleSnapshot initial = p_session.snapshot();
		if (initial.phase == BattlePhase::Terminal)
		{
			return {.operation = AIDriverOperation::Terminal};
		}
		if (initial.phase != BattlePhase::Activation || !initial.turn.has_value() || !initial.activeUnit.has_value())
		{
			return {};
		}
		const BattleUnitSnapshot *actor = activeSnapshotUnit(initial);
		if (actor == nullptr)
		{
			return {};
		}

		// The issuer/side pairing is still authoritatively checked by submit.  Keeping this early
		// check makes a misconfigured pump inert rather than trying to end another controller's turn.
		const bool expectedEnemy = p_issuer.controller == CommandController::EnemyAi;
		const bool expectedDebugPlayer = p_issuer.controller == CommandController::DebugAutoplay;
		if ((expectedEnemy && actor->side != BattleSide::Enemy) || (expectedDebugPlayer && actor->side != BattleSide::Player) ||
			(!expectedEnemy && !expectedDebugPlayer))
		{
			return {};
		}

		AIActivationGuardState &guard = _guardFor(p_session);
		if (guard.acceptedNonEndCommands >= p_session.registries().gameRules().battle.maxAiCommandsPerActivation)
		{
			return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiCommandCap);
		}

		const std::string_view profileId = p_behaviourOverride.has_value()
											   ? *p_behaviourOverride
											   : (actor->aiBehaviourId.has_value() ? std::string_view(*actor->aiBehaviourId) : std::string_view());
		const AIBehaviourDefinition *behaviour = profileId.empty()
													 ? nullptr
													 : p_session.registries().aiBehaviours().tryGet(std::string(profileId));
		if (behaviour == nullptr)
		{
			return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiNoRuleProducedCommand);
		}

		AIPlanResult planResult = _planner.chooseNextCommand(p_session, actor->id, *behaviour);
		if (!std::holds_alternative<AIPlannedCommand>(planResult))
		{
			return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiNoRuleProducedCommand);
		}
		AIPlannedCommand planned = std::get<AIPlannedCommand>(std::move(planResult));

		if (isNonEnd(planned.command))
		{
			const std::uint64_t progress = p_session.gameplayProgressDigest();
			if (std::ranges::find(guard.observedProgressDigests, progress) != guard.observedProgressDigests.end())
			{
				return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiRepeatedStateGuard);
			}
			guard.observedProgressDigests.push_back(progress);
		}

		const std::uint64_t digestAtPlan = p_session.gameplayProgressDigest();
		std::vector<AICommandDiagnostic> diagnostics;
		for (int attempt = 0; attempt != 2; ++attempt)
		{
			const CommandResult submission = p_session.submit(planned.command, p_issuer);
			if (std::holds_alternative<AcceptedCommand>(submission))
			{
				if (isNonEnd(planned.command) && p_session.phase() == BattlePhase::Activation)
				{
					++guard.acceptedNonEndCommands;
				}
				_clearIfActivationChanged(p_session);
				return {.operation = AIDriverOperation::Accepted, .planned = std::move(planned), .commandResult = submission, .diagnostics = std::move(diagnostics)};
			}
			if (std::holds_alternative<AbortedCommand>(submission))
			{
				_clearIfActivationChanged(p_session);
				return {.operation = AIDriverOperation::Terminal, .planned = std::move(planned), .commandResult = submission, .diagnostics = std::move(diagnostics)};
			}

			const CommandRejection rejection = std::get<RejectedCommand>(submission).reason;
			const std::uint64_t digestAfterRejection = p_session.gameplayProgressDigest();
			AICommandDiagnostic diagnostic;
			diagnostic.battle = p_session.battleId();
			diagnostic.turn = initial.turn;
			diagnostic.unit = actor->id;
			diagnostic.behaviourId = planned.behaviourId;
			diagnostic.ruleId = planned.ruleId;
			diagnostic.command = planned.command;
			diagnostic.rejection = rejection;
			diagnostic.progressDigest = digestAfterRejection;
			diagnostics.push_back(std::move(diagnostic));
			if (attempt == 0 && digestAfterRejection != digestAtPlan && p_session.phase() == BattlePhase::Activation)
			{
				planResult = _planner.chooseNextCommand(p_session, actor->id, *behaviour);
				if (std::holds_alternative<AIPlannedCommand>(planResult))
				{
					planned = std::get<AIPlannedCommand>(std::move(planResult));
					if (isNonEnd(planned.command))
					{
						const std::uint64_t freshProgress = p_session.gameplayProgressDigest();
						if (std::ranges::find(guard.observedProgressDigests, freshProgress) != guard.observedProgressDigests.end())
						{
							return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiRepeatedStateGuard, std::move(diagnostics));
						}
						guard.observedProgressDigests.push_back(freshProgress);
					}
					continue;
				}
			}
			return _submitEnd(p_session, actor->id, p_issuer, EndTurnRequestCause::AiInvalidPlannedCommand, std::move(diagnostics));
		}
		return {};
	}

	std::uint64_t BattleAIDriver::authoritativeStateDigest(const BattleSession &p_session) const
	{
		StableHasher64 hasher;
		mixU64(hasher, p_session.authoritativeBattleStateDigest());
		hasher.mix(static_cast<std::int32_t>(_guard.has_value() ? 1 : 0));
		if (_guard.has_value())
		{
			mixU64(hasher, _guard->turn.value);
			mixU64(hasher, _guard->unit.value());
			mixU64(hasher, _guard->acceptedNonEndCommands);
			mixU64(hasher, _guard->observedProgressDigests.size());
			for (const std::uint64_t observed : _guard->observedProgressDigests)
			{
				mixU64(hasher, observed);
			}
		}
		return hasher.value();
	}
}
