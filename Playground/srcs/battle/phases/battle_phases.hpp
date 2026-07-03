#pragma once

#include "battle/battle_action.hpp"
#include "battle/phases/i_battle_phase.hpp"

#include <cstdint>
#include <functional>
#include <memory>

namespace pg
{
	class BattleContext;
	class BattleUnit;
	class SetupPhase final : public IBattlePhase
	{
		std::function<void()> _done;

	public:
		explicit SetupPhase(std::function<void()> p) :
			_done(std::move(p))
		{
		}
		void enter() override;
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "Setup";
		}
	};
	class PlacementPhase final : public IBattlePhase
	{
		BattleContext &_context;
		std::uint32_t _seed;
		std::function<void(bool)> _done;

	public:
		PlacementPhase(BattleContext &c, std::uint32_t s, std::function<void(bool)> d) :
			_context(c),
			_seed(s),
			_done(std::move(d))
		{
		}
		void enter() override;
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "Placement";
		}
	};
	class IdlePhase final : public IBattlePhase
	{
		BattleContext &_context;
		std::function<void(BattleUnit *)> _ready;

	public:
		IdlePhase(BattleContext &c, std::function<void(BattleUnit *)> r) :
			_context(c),
			_ready(std::move(r))
		{
		}
		void enter() override;
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "Idle";
		}
	};
	class PlayerTurnPhase final : public IBattlePhase
	{
		std::function<void(std::unique_ptr<BattleAction>)> _chosen;
		bool _active = false;

	public:
		explicit PlayerTurnPhase(std::function<void(std::unique_ptr<BattleAction>)> c) :
			_chosen(std::move(c))
		{
		}
		void enter() override
		{
			_active = true;
		}
		void tick(float) override
		{
		}
		void exit() override
		{
			_active = false;
		}
		std::string_view name() const noexcept override
		{
			return "PlayerTurn";
		}
		[[nodiscard]] bool submitAction(std::unique_ptr<BattleAction> p_action);
		[[nodiscard]] bool active() const noexcept
		{
			return _active;
		}
	};
	class EnemyTurnPhase final : public IBattlePhase
	{
		BattleContext &_context;
		std::function<void(std::unique_ptr<BattleAction>)> _chosen;

	public:
		EnemyTurnPhase(BattleContext &c, std::function<void(std::unique_ptr<BattleAction>)> d) :
			_context(c),
			_chosen(std::move(d))
		{
		}
		void enter() override;
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "EnemyTurn";
		}
	};
	class ResolutionPhase final : public IBattlePhase
	{
		BattleContext &_context;
		std::unique_ptr<BattleAction> _pending;
		std::function<void(BattleActionKind, bool)> _done;

	public:
		ResolutionPhase(BattleContext &c, std::function<void(BattleActionKind, bool)> d) :
			_context(c),
			_done(std::move(d))
		{
		}
		void setPending(std::unique_ptr<BattleAction> p)
		{
			_pending = std::move(p);
		}
		void enter() override;
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "Resolution";
		}
	};
	class EndPhase final : public IBattlePhase
	{
		std::function<void()> _entered;

	public:
		explicit EndPhase(std::function<void()> e) :
			_entered(std::move(e))
		{
		}
		void enter() override
		{
			_entered();
		}
		void tick(float) override
		{
		}
		void exit() override
		{
		}
		std::string_view name() const noexcept override
		{
			return "End";
		}
	};
}
