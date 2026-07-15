#pragma once

#include "battle/presentation/battle_hud_view_model.hpp"
#include "widgets/battle/creature_card_widget.hpp"
#include "widgets/battle/team_roster_widget.hpp"

#include "structures/widget/spk_message_box.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"

#include <memory>
#include <vector>

	namespace pg
{
	class AbilitySlotWidget;
	class BattleInteractionController;
	class ResourceBarWidget;
	class StatusEffectSlotWidget;

	// Persistent overlay widget.  It has no BattleSession pointer: binding supplies intents to the
	// interaction controller and rendering consumes a fully-owned view model.
	class BattleHudWidget final : public spk::Widget
	{
	private:
		TeamRosterWidget<CreatureCardWidget> _playerRoster;
		TeamRosterWidget<CreatureCardWidget> _enemyRoster;
		spk::Panel _activePanel;
		spk::Panel _statusPanel;
		spk::TextLabel _status;
		spk::TextLabel _forecast;
		spk::PushButton _contextButton;
		spk::PushButton::Contract _contextContract;
		std::unique_ptr<ResourceBarWidget> _health;
		std::unique_ptr<ResourceBarWidget> _actionPoints;
		std::unique_ptr<ResourceBarWidget> _movementPoints;
		std::vector<std::unique_ptr<StatusEffectSlotWidget>> _effects;
		std::vector<std::unique_ptr<AbilitySlotWidget>> _abilities;
		std::unique_ptr<spk::MessageBox> _creatureDetailsWindow;
		std::unique_ptr<spk::MessageBox> _abilityDetailsWindow;
		std::unique_ptr<spk::MessageBox> _effectDetailsWindow;
		BattleInteractionController *_interaction = nullptr;
		std::optional<BattleHudViewModel> _model;

		void _layout();
		void _bindCallbacks();
		void _openDetails(const CreatureCardViewModel &p_model);
		void _openAbilityDetails(const AbilitySlotViewModel &p_model);
		void _openEffectDetails(const StatusViewModel &p_model);
		void _layoutWindow(spk::MessageBox &p_window, std::size_t p_maximumWidth, std::size_t p_maximumHeight);

	protected:
		void _onGeometryChange() override;

	public:
		explicit BattleHudWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		~BattleHudWidget() override;
		void bind(BattleInteractionController &p_interaction);
		void unbind() noexcept;
		void render(const BattleHudViewModel &p_model);
		[[nodiscard]] const std::optional<BattleHudViewModel> &model() const noexcept;
	};
}
