#include "widgets/battle/ability_slot_widget.hpp"

#include "structures/widget/spk_widget_style.hpp"

namespace pg
{
	AbilitySlotWidget::AbilitySlotWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_button(p_name + "/button", this)
	{
		_clickContract = _button.subscribeToClick([this] {
			invokeEnabledClick();
		});
		activate();
	}

	void AbilitySlotWidget::_applyVisualState()
	{
		const spk::Color color{1.0f, 1.0f, 1.0f, 1.0f};
		_button.releasedLabel().setGlyphColor(color);
		_button.pressedLabel().setGlyphColor(color);
	}

	void AbilitySlotWidget::_onGeometryChange()
	{
		_button.setGeometry({{0, 0}, geometry().width(), geometry().height()});
	}

	void AbilitySlotWidget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		if (p_event->button == spk::Mouse::Right && absoluteGeometry().contains(p_event.device().position))
		{
			invokeDetailsClick();
			p_event.consume();
		}
	}

	void AbilitySlotWidget::render(const AbilitySlotViewModel &p_model)
	{
		_enabled = p_model.enabled;
		_abilityId = p_model.abilityId;
		std::string text = p_model.abilityId.has_value() ? std::to_string(p_model.shortcut) : "-";
		if (p_model.selected)
		{
			text = "[" + text + "]";
		}
		_button.setText(text);
		if (p_model.icon.has_value())
		{
			const std::shared_ptr<spk::SpriteSheet> &iconset = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
			if (iconset != nullptr)
			{
				_button.setIcon(iconset, iconset->spriteID({(*p_model.icon)[0], (*p_model.icon)[1]}));
			}
			else
			{
				_button.removeIcon();
			}
		}
		else
		{
			_button.removeIcon();
		}
		_applyVisualState();
	}

	void AbilitySlotWidget::setOnEnabledClick(std::function<void()> p_callback)
	{
		_onEnabledClick = std::move(p_callback);
	}
	void AbilitySlotWidget::setOnDetailsClick(std::function<void()> p_callback)
	{
		_onDetailsClick = std::move(p_callback);
	}
	void AbilitySlotWidget::invokeEnabledClick()
	{
		if (_enabled && _abilityId.has_value() && _onEnabledClick)
		{
			_onEnabledClick();
		}
	}
	void AbilitySlotWidget::invokeDetailsClick()
	{
		if (_abilityId.has_value() && _onDetailsClick)
		{
			_onDetailsClick();
		}
	}
	bool AbilitySlotWidget::enabled() const noexcept
	{
		return _enabled;
	}
}
