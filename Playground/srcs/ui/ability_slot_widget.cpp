#include "ui/ability_slot_widget.hpp"

#include "abilities/ability.hpp"

#include <string>
#include <utility>

namespace pg
{
	AbilitySlotWidget::AbilitySlotWidget(
		const std::string &p_name,
		std::size_t p_slot,
		std::shared_ptr<spk::SpriteSheet> p_icons,
		spk::Widget *p_parent) :
		spk::PushButton(p_name, p_parent),
		_slot(p_slot),
		_icons(std::move(p_icons)),
		_clickContract(subscribeToClick([this] {
			if (_enabled && _ability != nullptr && _selection)
			{
				_selection(_slot);
			}
		}))
	{
		setAbility(nullptr, false, false);
	}

	void AbilitySlotWidget::setSelectionHandler(std::function<void(std::size_t)> p_selection)
	{
		_selection = std::move(p_selection);
	}

	void AbilitySlotWidget::setHoverHandler(std::function<void(const Ability *)> p_hover)
	{
		_hover = std::move(p_hover);
	}

	void AbilitySlotWidget::setAbility(const Ability *p_ability, bool p_enabled, bool p_selected)
	{
		_ability = p_ability;
		_enabled = p_enabled && p_ability != nullptr;
		_selected = p_selected && p_ability != nullptr;
		if (_ability == nullptr)
		{
			setText("[" + std::to_string(_slot + 1) + "]\n-");
			removeIcon();
		}
		else
		{
			setText("[" + std::to_string(_slot + 1) + "] " + _ability->displayName +
				"\nAP " + std::to_string(_ability->apCost) + "  MP " + std::to_string(_ability->mpCost));
			if (_icons != nullptr)
			{
				setIcon(_icons, _icons->spriteID({
					static_cast<unsigned int>(_ability->icon[0]),
					static_cast<unsigned int>(_ability->icon[1])}));
				setIconSize({24, 24});
			}
		}
		_refreshLook();
	}

	void AbilitySlotWidget::_refreshLook()
	{
		const spk::Color color = _selected
			? spk::Color{1.0f, 0.9f, 0.25f, 1.0f}
			: (_enabled ? spk::Color{1.0f, 1.0f, 1.0f, 1.0f} : spk::Color{0.45f, 0.45f, 0.45f, 1.0f});
		releasedLabel().setGlyphColor(color);
		pressedLabel().setGlyphColor(color);
	}

	void AbilitySlotWidget::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		const bool wasHovered = isHovered();
		spk::PushButton::_onMouseMovedEvent(p_event);
		if (_hover && wasHovered != isHovered()) _hover(isHovered() ? _ability : nullptr);
	}

	const Ability *AbilitySlotWidget::ability() const noexcept { return _ability; }
	bool AbilitySlotWidget::enabled() const noexcept { return _enabled; }
	bool AbilitySlotWidget::selected() const noexcept { return _selected; }
}
