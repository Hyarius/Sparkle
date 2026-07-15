#include "widgets/battle/status_effect_slot_widget.hpp"

#include "structures/widget/spk_widget_style.hpp"

#include <algorithm>

namespace pg
{
	StatusEffectSlotWidget::StatusEffectSlotWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_button(p_name + "/button", this)
	{
		const spk::Color white{1.0f, 1.0f, 1.0f, 1.0f};
		_button.releasedLabel().setGlyphColor(white);
		_button.pressedLabel().setGlyphColor(white);
		activate();
	}

	void StatusEffectSlotWidget::_onGeometryChange()
	{
		_button.setGeometry({{0, 0}, geometry().width(), geometry().height()});
	}

	void StatusEffectSlotWidget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		if (p_event->button == spk::Mouse::Right && absoluteGeometry().contains(p_event.device().position))
		{
			invokeDetailsClick();
			p_event.consume();
		}
	}

	void StatusEffectSlotWidget::render(const StatusViewModel &p_model)
	{
		_model = p_model;
		std::string text = _model.displayName.empty() ? _model.id : _model.displayName;
		text.resize(std::min<std::size_t>(text.size(), 2));
		if (_model.stacks > 1)
		{
			text += "x" + std::to_string(_model.stacks);
		}
		_button.setText(text);
		if (_model.icon.has_value())
		{
			const std::shared_ptr<spk::SpriteSheet> &iconset = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();
			if (iconset != nullptr)
			{
				_button.setIcon(iconset, iconset->spriteID({(*_model.icon)[0], (*_model.icon)[1]}));
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
	}

	void StatusEffectSlotWidget::setOnDetailsClick(std::function<void()> p_callback)
	{
		_onDetailsClick = std::move(p_callback);
	}

	void StatusEffectSlotWidget::invokeDetailsClick()
	{
		if (_onDetailsClick)
		{
			_onDetailsClick();
		}
	}
}
