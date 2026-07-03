#pragma once

#include "core/mode.hpp"

#include <memory>

namespace pg
{
	class BattleContext;
	class BattleCoordinator;
	class BattleScene;

	class BattleMode : public Mode
	{
	private:
		BattleContext *_battleContext = nullptr;
		BattleScene *_scene = nullptr;
		std::unique_ptr<BattleCoordinator> _coordinator;

	protected:
		virtual void activate();
		virtual void deactivate();

	public:
		explicit BattleMode(GameContext &p_context);
		~BattleMode() override;

		void enter() override;
		void exit() override;
		void update(const spk::UpdateTick &p_tick) override;
		void setBattleContext(BattleContext *p_context) noexcept;
		// Presentation glue, injected by GameSceneWidget once the engine/camera/world exist.
		void setScene(BattleScene *p_scene) noexcept;
		[[nodiscard]] BattleCoordinator *coordinator() noexcept;
		[[nodiscard]] bool presentResult(BattleSide p_winner);
	};
}
