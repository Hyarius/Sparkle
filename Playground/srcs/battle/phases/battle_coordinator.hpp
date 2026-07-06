#pragma once

#include "battle/battle_side.hpp"
#include "battle/phases/battle_orchestrator.hpp"
#include "battle/phases/battle_phases.hpp"

#include <optional>

namespace pg
{
	class BattleContext;
	class BattleCoordinator
	{
	private:
		BattleContext &_context;
		BattleOrchestrator _orchestrator;
		SetupPhase _setup;
		PlacementPhase _placement;
		IdlePhase _idle;
		PlayerTurnPhase _player;
		EnemyTurnPhase _enemy;
		ResolutionPhase _resolution;
		EndPhase _end;
		bool _finished = false;
		std::optional<BattleSide> _winner;
		void _onAction(std::unique_ptr<BattleAction> p_action);
		void _afterResolution(BattleActionKind p_kind, bool p_success);
		void _finish();

	public:
		BattleCoordinator(
			BattleContext &p_context,
			std::uint32_t p_seed = 0,
			bool p_interactivePlacement = false);
		~BattleCoordinator();
		void start();
		void tick(float p_seconds);
		[[nodiscard]] PlacementPhase &placementPhase() noexcept;
		[[nodiscard]] PlayerTurnPhase &playerTurnPhase() noexcept;
		[[nodiscard]] bool finished() const noexcept;
		[[nodiscard]] std::optional<BattleSide> winner() const noexcept;
		[[nodiscard]] std::string_view currentPhaseName() const noexcept;
	};
}
