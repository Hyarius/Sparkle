#include "ui/creature_info_window.hpp"

#include "abilities/ability.hpp"
#include "battle/battle_context.hpp"
#include "battle/battle_statuses.hpp"
#include "battle/battle_unit.hpp"
#include "core/event_center.hpp"
#include "creatures/creature_unit.hpp"
#include "statuses/status.hpp"
#include "taming/taming_profile.hpp"
#include "taming/wild_battle_unit.hpp"
#include "ui/ability_card_widget.hpp"
#include "ui/passive_card_widget.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace pg
{
	CreatureInfoWindow::CreatureInfoWindow(const std::string &p_name, spk::Widget *p_parent) :
		spk::InterfaceWindow<spk::Panel>(p_name, p_parent),
		_stats(p_name + "/Stats", &contentObject()),
		_abilities(p_name + "/Abilities", &contentObject()),
		_passives(p_name + "/Passives", &contentObject()),
		_taming(p_name + "/Taming", &contentObject())
	{
		setTitle("Creature information");
		deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Minimize);
		deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Maximize);
		_stats.setTextSize(spk::Font::Size(13, 1));
		_abilities.setTextSize(spk::Font::Size(12, 1));
		_passives.setTextSize(spk::Font::Size(12, 1));
		_taming.setTextSize(spk::Font::Size(12, 1));
		deactivate();
	}

	CreatureInfoWindow::Content CreatureInfoWindow::contentFor(const BattleUnit &p_unit)
	{
		Content result;
		const auto *wild = dynamic_cast<const WildBattleUnit *>(&p_unit);
		result.kind = p_unit.side == BattleSide::Player
			? ContentKind::Owned
			: (wild != nullptr ? ContentKind::WildEnemy : ContentKind::TrainerEnemy);

		std::ostringstream stats;
		stats << "HP " << p_unit.attributes.hp.current() << '/' << p_unit.attributes.hp.max()
			<< "   AP " << p_unit.attributes.ap.current() << '/' << p_unit.attributes.ap.max()
			<< "   MP " << p_unit.attributes.mp.current() << '/' << p_unit.attributes.mp.max() << '\n'
			<< "ATK " << p_unit.attributes.attack.value()
			<< "   Armor " << p_unit.attributes.armor.value()
			<< "   Magic " << p_unit.attributes.magic.value()
			<< "   Resist " << p_unit.attributes.resistance.value();
		result.stats = stats.str();

		std::ostringstream abilities;
		abilities << "Abilities";
		if (p_unit.source() != nullptr)
		{
			for (const Ability *ability : p_unit.source()->abilities)
			{
				if (ability != nullptr)
				{
					abilities << "\n- " << ability->displayName << ": "
						<< AbilityCardWidget::rulesText(*ability);
				}
			}
		}
		result.abilities = abilities.str();

		std::ostringstream passives;
		passives << "Passives and statuses";
		for (const BattleStatus &entry : p_unit.statuses.entries())
		{
			if (entry.definition != nullptr)
			{
				passives << "\n- " << entry.definition->displayName << " x" << entry.stacks
					<< ": " << PassiveCardWidget::rulesText(*entry.definition);
			}
		}
		result.passives = passives.str();

		if (wild != nullptr)
		{
			const TamingProfile *profile = wild->tamingProgress.profile();
			const bool revealed = std::ranges::any_of(
				wild->tamingProgress.advancements, [](const BattleCondition::Advancement &p_value) {
					return p_value.progress > 0.0f || p_value.completedRepeatCount > 0;
				});
			std::ostringstream taming;
			taming << "Taming conditions";
			const std::size_t count = profile != nullptr ? profile->conditions.size() : 0;
			for (std::size_t index = 0; index < count; ++index)
			{
				taming << "\n- Condition " << index + 1 << ": ";
				if (!revealed)
				{
					taming << "???";
					continue;
				}
				const BattleCondition::Advancement progress = index < wild->tamingProgress.advancements.size()
					? wild->tamingProgress.advancements[index]
					: BattleCondition::Advancement{};
				const int required = profile->conditions[index]->requiredRepeatCount;
				taming << static_cast<int>(std::floor(progress.progress)) << "%"
					<< " (" << progress.completedRepeatCount << '/' << required << ')';
			}
			result.taming = taming.str();
		}
		return result;
	}

	void CreatureInfoWindow::bind(BattleContext &p_context, BattleUnit &p_unit)
	{
		unbind();
		_context = &p_context;
		_unit = &p_unit;
		_battleEventContract = _context->events().battleEventOccurred.subscribe(
			[this](const BattleEvent *) { _refresh(); });
		setTitle(p_unit.displayName());
		_refresh();
		activate();
	}

	void CreatureInfoWindow::unbind()
	{
		_battleEventContract.resign();
		_context = nullptr;
		_unit = nullptr;
		_content = {};
		deactivate();
	}

	void CreatureInfoWindow::_refresh()
	{
		if (_unit == nullptr) return;
		_content = contentFor(*_unit);
		_stats.setText(_content.stats);
		_abilities.setText(_content.abilities);
		_passives.setText(_content.passives);
		_taming.setText(_content.taming);
		if (_content.taming.empty()) _taming.deactivate(); else _taming.activate();
	}

	void CreatureInfoWindow::_onGeometryChange()
	{
		spk::InterfaceWindow<spk::Panel>::_onGeometryChange();
		const spk::Vector2UInt size = contentSize();
		_stats.setGeometry(spk::Rect2D(8, 6, size.x > 16 ? size.x - 16 : 0, 54));
		_abilities.setGeometry(spk::Rect2D(8, 64, size.x > 16 ? size.x - 16 : 0, 118));
		_passives.setGeometry(spk::Rect2D(8, 186, size.x > 16 ? size.x - 16 : 0, 98));
		_taming.setGeometry(spk::Rect2D(8, 288, size.x > 16 ? size.x - 16 : 0, size.y > 296 ? size.y - 296 : 0));
	}

	BattleUnit *CreatureInfoWindow::unit() const noexcept { return _unit; }
	const CreatureInfoWindow::Content &CreatureInfoWindow::displayedContent() const noexcept { return _content; }
}
