#pragma once

#include "battle/battle_event_log.hpp"
#include "battle/battle_snapshot.hpp"
#include "battle/presentation/battle_interaction_controller.hpp"
#include "battle/presentation/battle_presentation_cell_source.hpp"

#include "structures/system/event/spk_events.hpp"
#include "structures/system/event/spk_update_context.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace spk
{
	class Camera3D;
	class GameEngine;
	class Texture;
}

namespace pg
{
	class BattleBoardPicker;
	class BattleObjectPresenter;
	class BattleOverlayModelBuilder;
	class BattleOverlayPresenter;
	class BattleSession;
	class BattleUnitPresenter;
	class BattleHudWidget;
	class TacticalCameraController;
	struct GameRules;

	using BattleSnapshotProvider = std::function<BattleSnapshot()>;

	struct BattlePresentationCallbacks
	{
		std::function<void()> presentationStateChanged;
		std::function<void(const CommandResult &)> commandCompleted;
	};

	class BattlePresentationRuntime
	{
	private:
		spk::GameEngine &_engine;
		spk::Camera3D &_camera;
		BattleSession &_commands;
		BattleSnapshotProvider _snapshots;
		BattlePresentationCallbacks _callbacks;
		std::function<spk::Vector2()> _viewportSize;
		const spk::Texture &_maskTexture;
		const spk::Texture &_unitTexture;
		const GameRules &_rules;
		BattleHudWidget &_hud;
		const BattlePresentationBoardBinding *_binding = nullptr;
		std::unique_ptr<BattleOverlayPresenter> _presenter;
		std::unique_ptr<BattleUnitPresenter> _unitPresenter;
		std::unique_ptr<BattleObjectPresenter> _objectPresenter;
		std::unique_ptr<BattleInteractionController> _interaction;
		std::unique_ptr<BattleOverlayModelBuilder> _overlay;
		std::unique_ptr<BattleBoardPicker> _picker;
		std::unique_ptr<TacticalCameraController> _cameraController;
		BattleSnapshot _lastSnapshot;
		bool _entered = false;
		bool _rightPressed = false;
		bool _rightDragging = false;
		spk::Vector2Int _rightPressPosition{};

		void _refreshOverlay();
		void _refreshHud();
		[[nodiscard]] std::optional<BoardCell> _pick(const spk::Vector2Int &p_mouse) const;
		[[nodiscard]] bool _handled(InputHandling p_result);

	public:
		BattlePresentationRuntime(
			spk::GameEngine &p_engine,
			spk::Camera3D &p_camera,
			BattleSession &p_commands,
			BattleSnapshotProvider p_snapshots,
			BattlePresentationCallbacks p_callbacks,
			std::function<spk::Vector2()> p_viewportSize,
			const spk::Texture &p_maskTexture,
			const spk::Texture &p_unitTexture,
			const GameRules &p_rules,
			BattleHudWidget &p_hud);
		~BattlePresentationRuntime();
		BattlePresentationRuntime(const BattlePresentationRuntime &) = delete;
		BattlePresentationRuntime &operator=(const BattlePresentationRuntime &) = delete;

		void enter(const BattlePresentationBoardBinding &p_boardPresentation);
		void onCommittedBatch(const CommittedBattleBatch &p_batch);
		void update(const spk::UpdateContext &p_tick);
		void beginExit();
		void detach() noexcept;

		[[nodiscard]] bool onMouseMoved(const spk::MouseMovedEvent &p_event);
		[[nodiscard]] bool onMousePressed(const spk::MouseButtonPressedEvent &p_event);
		[[nodiscard]] bool onMouseReleased(const spk::MouseButtonReleasedEvent &p_event);
		[[nodiscard]] bool onMouseWheel(const spk::MouseWheelScrolledEvent &p_event);
		[[nodiscard]] bool onKeyPressed(const spk::KeyPressedEvent &p_event);
		void onMouseLeave() noexcept;

		[[nodiscard]] bool entered() const noexcept;
		[[nodiscard]] const BattleInteractionController *interaction() const noexcept;
	};
}
