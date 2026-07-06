#pragma once

#include "battle/battle_event.hpp"
#include "core/event_center.hpp"
#include "structures/widget/spk_widget.hpp"
#include "ui/active_unit_panel.hpp"
#include "ui/ability_card_widget.hpp"
#include "ui/creature_info_window.hpp"
#include "ui/placement_ui.hpp"
#include "ui/team_column_widget.hpp"
#include "ui/turn_order_strip.hpp"

#include <memory>

namespace pg
{
	class BattleContext;
	class BattleInput;
	class BattleCoordinator;

	class BattleHud : public spk::Widget
	{
	private:
		TeamColumnWidget _playerTeam;
		TeamColumnWidget _enemyTeam;
		TurnOrderStrip _turnOrder;
		ActiveUnitPanel _activeUnit;
		PlacementUI _placement;
		AbilityCardWidget _abilityPreview;
		CreatureInfoWindow _creatureInfo;
		BattleContext *_context = nullptr;
		BattleInput *_input = nullptr;
		spk::ContractProvider<const BattleEvent *>::Contract _battleEventContract;
		spk::ContractProvider<BattleUnit *>::Contract _turnEndedContract;

		void _bindActiveUnit(BattleUnit *p_unit);
		void _refresh();

	protected:
		void _onGeometryChange() override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;

	public:
		BattleHud(
			const std::string &p_name,
			std::shared_ptr<spk::SpriteSheet> p_icons,
			spk::Widget *p_parent = nullptr);

		void bind(BattleContext &p_context, BattleInput &p_input, BattleCoordinator &p_coordinator);
		void unbind();
		[[nodiscard]] bool bound() const noexcept;
		[[nodiscard]] const TeamColumnWidget &playerTeam() const noexcept;
		[[nodiscard]] const TeamColumnWidget &enemyTeam() const noexcept;
		[[nodiscard]] const TurnOrderStrip &turnOrder() const noexcept;
		[[nodiscard]] const ActiveUnitPanel &activeUnit() const noexcept;
		[[nodiscard]] const PlacementUI &placement() const noexcept;
		[[nodiscard]] const CreatureInfoWindow &creatureInfo() const noexcept;
	};
}
