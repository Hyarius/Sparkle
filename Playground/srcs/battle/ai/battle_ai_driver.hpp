#pragma once

#include "battle/ai/battle_ai_planner.hpp"
#include "battle/battle_command_result.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pg
{
	class BattleSession;

	// Orchestration scratch for one active AI activation.  It is deliberately outside the battle
	// snapshot: it is not a gameplay fact, but it does affect deterministic future control flow.
	struct AIActivationGuardState
	{
		TurnIndex turn;
		BattleUnitId unit;
		std::uint32_t acceptedNonEndCommands = 0;
		std::vector<std::uint64_t> observedProgressDigests;

		[[nodiscard]] bool operator==(const AIActivationGuardState &) const = default;
	};

	struct AICommandDiagnostic
	{
		BattleId battle;
		std::optional<TurnIndex> turn;
		BattleUnitId unit;
		std::string behaviourId;
		std::string ruleId;
		BattleCommand command;
		CommandRejection rejection = CommandRejection::CommandUnavailable;
		std::uint64_t progressDigest = 0;

		[[nodiscard]] bool operator==(const AICommandDiagnostic &) const = default;
	};

	enum class AIDriverOperation
	{
		NoOperation,
		Accepted,
		SafelyEnded,
		Terminal
	};

	struct AIDriverResult
	{
		AIDriverOperation operation = AIDriverOperation::NoOperation;
		std::optional<AIPlannedCommand> planned;
		std::optional<CommandResult> commandResult;
		std::vector<AICommandDiagnostic> diagnostics;
	};

	// Performs at most one ordinary command submission.  It owns no battle state and has no
	// resolver access; every mutation below still crosses BattleSession::submit.
	class BattleAIDriver
	{
	private:
		BattleAIPlanner _planner;
		std::optional<AIActivationGuardState> _guard;

		void _clearIfActivationChanged(const BattleSession &p_session);
		[[nodiscard]] AIActivationGuardState &_guardFor(const BattleSession &p_session);
		[[nodiscard]] AIDriverResult _submitEnd(
			BattleSession &p_session,
			BattleUnitId p_unit,
			CommandIssuer p_issuer,
			EndTurnRequestCause p_cause,
			std::vector<AICommandDiagnostic> p_diagnostics = {});

	public:
		// The override is development-only caller-owned configuration.  Passing one is how the
		// pump autoplays a player activation; an enemy unit otherwise uses its copied encounter id.
		[[nodiscard]] AIDriverResult executeOne(
			BattleSession &p_session,
			CommandIssuer p_issuer,
			std::optional<std::string_view> p_behaviourOverride = std::nullopt);

		[[nodiscard]] const std::optional<AIActivationGuardState> &guardState() const noexcept
		{
			return _guard;
		}
		[[nodiscard]] std::uint64_t authoritativeStateDigest(const BattleSession &p_session) const;
	};
}
