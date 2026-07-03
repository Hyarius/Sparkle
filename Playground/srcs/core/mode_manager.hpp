#pragma once

#include "core/mode.hpp"

#include <memory>

namespace pg
{
	class BattleMode;

	class ModeManager
	{
	private:
		GameContext &_context;
		std::unique_ptr<Mode> _explorationMode;
		std::unique_ptr<Mode> _battleMode;
		Mode *_currentMode = nullptr;
		spk::ContractProvider<BattleContext *>::Contract _battleStartedContract;
		spk::ContractProvider<BattleContext *, BattleSide>::Contract _battleResolvedContract;
		spk::ContractProvider<>::Contract _battleEndConfirmedContract;
		BattleContext *_resolvedContext = nullptr;

		void _completeBattle();

	public:
		explicit ModeManager(GameContext &p_context);
		ModeManager(
			GameContext &p_context,
			std::unique_ptr<Mode> p_explorationMode,
			std::unique_ptr<Mode> p_battleMode);
		~ModeManager();

		ModeManager(const ModeManager &) = delete;
		ModeManager &operator=(const ModeManager &) = delete;

		void switchTo(Mode &p_mode);
		void enterExploration();
		void enterBattle();
		void update(const spk::UpdateTick &p_tick);

		[[nodiscard]] Mode *currentMode() noexcept;
		[[nodiscard]] const Mode *currentMode() const noexcept;
		[[nodiscard]] BattleMode *battleMode() noexcept;
	};
}
