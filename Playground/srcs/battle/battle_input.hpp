#pragma once

#include "battle/battle_action.hpp"
#include "battle/board_overlay_state.hpp"
#include "board/traversal_graph.hpp"
#include "components/actor.hpp"
#include "structures/game_engine/spk_camera_3d.hpp"
#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace pg
{
	class BattleContext;
	class BattleCoordinator;
	struct Ability;

	// Headless heart of battle targeting: consumes hover/click/key intents (board-local cells),
	// writes the preview into a BoardOverlayState, and returns the chosen BattleAction. No engine,
	// window, or picking here so the mode/preview state machine is unit-testable against a scripted
	// battle (tests/battle/battle_input_test.cpp). BattleInput below wraps it with mouse picking.
	class BattleInputController
	{
	public:
		enum class Mode
		{
			Move,	// hover -> path + move range; click -> MoveAction
			Ability // hover -> AoE + range/LoS shading; click -> AbilityAction
		};

	private:
		const BattleContext &_context;
		BoardOverlayState &_overlay;
		Mode _mode = Mode::Move;
		const Ability *_ability = nullptr;
		std::optional<spk::Vector3Int> _hovered;
		std::map<spk::Vector3Int, float, CellPositionLess> _reachable;
		std::vector<spk::Vector3Int> _validTargets;

		[[nodiscard]] BattleUnit *_activeUnit() const;
		void _refresh();

	public:
		BattleInputController(const BattleContext &p_context, BoardOverlayState &p_overlay);

		void reset(); // back to Move mode, clears the preview
		void enterMoveMode();
		[[nodiscard]] bool enterAbilityMode(std::size_t p_index = 0); // selects active unit ability
		void handleDigit(int p_digit);								  // 1..9 -> select the matching ability
		void escape();												  // -> Move mode
		void hover(const std::optional<spk::Vector3Int> &p_cell);
		[[nodiscard]] std::unique_ptr<BattleAction> click(const std::optional<spk::Vector3Int> &p_cell);
		[[nodiscard]] std::unique_ptr<BattleAction> endTurn();

		[[nodiscard]] Mode mode() const noexcept
		{
			return _mode;
		}
		[[nodiscard]] const Ability *selectedAbility() const noexcept
		{
			return _ability;
		}
		[[nodiscard]] const std::optional<spk::Vector3Int> &hovered() const noexcept
		{
			return _hovered;
		}
	};

	// Engine input logic active only during the player's turn: picks the board cell under the
	// mouse, drives a BattleInputController, and forwards chosen actions to PlayerTurnPhase.
	class BattleInput : public spk::ComponentLogic<Actor>
	{
	public:
		using ViewportSize = std::function<spk::Vector2()>;

	private:
		spk::Camera3D &_camera;
		BoardOverlayState &_overlay;
		ViewportSize _viewportSize;
		BattleContext *_context = nullptr;
		BattleCoordinator *_coordinator = nullptr;
		std::unique_ptr<BattleInputController> _controller;
		std::function<bool()> _busy = [] {
			return false;
		};

		[[nodiscard]] bool _isPlayerInputActive() const;
		[[nodiscard]] std::optional<spk::Vector3Int> _pick(const spk::Vector2Int &p_mouse) const;
		void _submit(std::unique_ptr<BattleAction> p_action);

	public:
		BattleInput(spk::Camera3D &p_camera, BoardOverlayState &p_overlay, ViewportSize p_viewportSize);

		void bind(BattleContext &p_context, BattleCoordinator &p_coordinator);
		void unbind();
		void setBusyPredicate(std::function<bool()> p_busy);

	protected:
		void _parseComponentForMouseMovedEvent(spk::MouseMovedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event, Actor &p_actor) override;
		void _parseComponentForKeyPressedEvent(spk::KeyPressedEvent &p_event, Actor &p_actor) override;
	};
}
