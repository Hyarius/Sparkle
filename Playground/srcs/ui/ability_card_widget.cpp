#include "ui/ability_card_widget.hpp"

#include "abilities/ability.hpp"
#include "abilities/effect.hpp"

#include <sstream>
#include <utility>

namespace
{
	std::string rangeName(pg::RangeShape p_shape)
	{
		switch (p_shape)
		{
		case pg::RangeShape::Circle: return "circle";
		case pg::RangeShape::Line: return "line";
		case pg::RangeShape::Diagonal: return "diagonal";
		case pg::RangeShape::Self: return "self";
		}
		return "unknown";
	}

	std::string areaName(pg::AreaShape p_shape)
	{
		switch (p_shape)
		{
		case pg::AreaShape::Square: return "square";
		case pg::AreaShape::Circle: return "circle";
		case pg::AreaShape::Cross: return "cross";
		case pg::AreaShape::Line: return "line";
		}
		return "unknown";
	}

	std::string targetName(pg::TargetProfile p_profile)
	{
		switch (p_profile)
		{
		case pg::TargetProfile::Everything: return "everything";
		case pg::TargetProfile::Ally: return "ally";
		case pg::TargetProfile::Enemy: return "enemy";
		case pg::TargetProfile::Empty: return "empty cell";
		}
		return "unknown";
	}
}

namespace pg
{
	AbilityCardWidget::AbilityCardWidget(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_icons(std::move(p_icons)),
		_icon(p_name + "/Icon", this),
		_name(p_name + "/Name", this),
		_metadata(p_name + "/Metadata", this),
		_rules(p_name + "/Rules", this)
	{
		_name.setTextSize(spk::Font::Size(17, 1));
		_metadata.setTextSize(spk::Font::Size(12, 1));
		_rules.setTextSize(spk::Font::Size(13, 1));
		bind(nullptr);
	}

	std::string AbilityCardWidget::metadataText(const Ability &p_ability)
	{
		return "AP " + std::to_string(p_ability.apCost) + "  MP " + std::to_string(p_ability.mpCost) +
			" | Range " + std::to_string(p_ability.minimumRange) + "-" +
			std::to_string(p_ability.maximumRange) + " " + rangeName(p_ability.rangeShape) +
			" | AoE " + areaName(p_ability.areaShape) + " " + std::to_string(p_ability.areaValue) +
			" | " + targetName(p_ability.targetProfile) +
			(p_ability.requiresLineOfSight ? " | LoS" : " | ignores LoS");
	}

	std::string AbilityCardWidget::rulesText(const Ability &p_ability)
	{
		std::ostringstream result;
		for (const auto &effect : p_ability.effects)
		{
			if (effect == nullptr) continue;
			if (result.tellp() > 0) result << ' ';
			result << describe(*effect);
		}
		if (result.tellp() == 0 && p_ability.baseDamage > 0)
		{
			result << "Deals " << p_ability.baseDamage << ' '
				<< (p_ability.damageKind == DamageKind::Physical ? "physical" : "magical")
				<< " damage.";
		}
		return result.str();
	}

	void AbilityCardWidget::bind(const Ability *p_ability)
	{
		_ability = p_ability;
		if (_ability == nullptr)
		{
			_name.setText("");
			_metadata.setText("");
			_rules.setText("");
			_icon.deactivate();
			deactivate();
			return;
		}
		_name.setText(_ability->displayName);
		_metadata.setText(metadataText(*_ability));
		_rules.setText(rulesText(*_ability));
		if (_icons != nullptr)
		{
			_icon.setTexture(_icons);
			_icon.setSection(_icons->sprite({
				static_cast<unsigned int>(_ability->icon[0]),
				static_cast<unsigned int>(_ability->icon[1])}));
			_icon.activate();
		}
		activate();
	}

	void AbilityCardWidget::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int width = geometry().width();
		_icon.setGeometry(spk::Rect2D(6, 6, 44, 44));
		_name.setGeometry(spk::Rect2D(56, 4, width > 62 ? width - 62 : 0, 24));
		_metadata.setGeometry(spk::Rect2D(56, 28, width > 62 ? width - 62 : 0, 24));
		_rules.setGeometry(spk::Rect2D(6, 56, width > 12 ? width - 12 : 0, geometry().height() > 62 ? geometry().height() - 62 : 0));
	}

	const Ability *AbilityCardWidget::ability() const noexcept { return _ability; }
}
