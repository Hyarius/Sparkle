#include "structures/widget/spk_push_button.hpp"

#include <utility>

namespace spk
{
	PushButton::PushButton(const std::string& p_name, spk::Widget* p_parent) :
		PushButton(p_name, "", p_parent)
	{
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		spk::Widget* p_parent) :
		PushButton(
			p_name,
			p_text,
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed),
			p_parent)
	{
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		const spk::WidgetStyle& p_style,
		spk::Widget* p_parent) :
		PushButton(p_name, p_text, p_style, p_style, p_parent)
	{
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		const spk::WidgetStyle& p_releasedStyle,
		const spk::WidgetStyle& p_pressedStyle,
		spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent),
		_releasedBackground(p_name + "::releasedBackground", p_releasedStyle, this),
		_pressedBackground(p_name + "::pressedBackground", p_pressedStyle, this),
		_releasedLabel(p_name + "::releasedLabel", p_text, p_releasedStyle, this),
		_pressedLabel(p_name + "::pressedLabel", p_text, p_pressedStyle, this)
	{
		_pressedBackground.deactivate();
		_pressedLabel.deactivate();
		activate();
	}

	void PushButton::applyStyle(const spk::WidgetStyle& p_style)
	{
		applyStyle(p_style, p_style);
	}

	void PushButton::applyStyle(const spk::WidgetStyle& p_releasedStyle, const spk::WidgetStyle& p_pressedStyle)
	{
		_releasedBackground.applyStyle(p_releasedStyle);
		_pressedBackground.applyStyle(p_pressedStyle);
		_releasedLabel.applyStyle(p_releasedStyle);
		_pressedLabel.applyStyle(p_pressedStyle);
	}

	void PushButton::_refreshState()
	{
		if (_isPressed)
		{
			_releasedBackground.deactivate();
			_releasedLabel.deactivate();
			_pressedBackground.activate();
			_pressedLabel.activate();
		}
		else
		{
			_releasedBackground.activate();
			_releasedLabel.activate();
			_pressedBackground.deactivate();
			_pressedLabel.deactivate();
		}
	}

	void PushButton::_onGeometryChange()
	{
		const spk::Rect2D childRect(0, 0, geometry().width(), geometry().height());
		_releasedBackground.setGeometry(childRect);
		_pressedBackground.setGeometry(childRect);
		_releasedLabel.setGeometry(childRect);
		_pressedLabel.setGeometry(childRect);
	}

	PushButton::Contract PushButton::subscribeToClick(PushButton::Callback p_callback)
	{
		return _clickProvider.subscribe(std::move(p_callback));
	}

	void PushButton::useDefaultStyles()
	{
		applyStyle(
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed));
	}

	void PushButton::setText(const spk::Font::Text& p_text)
	{
		_releasedLabel.setText(p_text);
		_pressedLabel.setText(p_text);
	}

	void PushButton::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void PushButton::setAlignment(spk::HorizontalAlignment p_horizontal, spk::VerticalAlignment p_vertical)
	{
		_releasedLabel.setAlignment(p_horizontal, p_vertical);
		_pressedLabel.setAlignment(p_horizontal, p_vertical);
	}

	bool PushButton::isHovered() const
	{
		return _isHovered;
	}

	bool PushButton::isPressed() const
	{
		return _isPressed;
	}

	spk::Panel& PushButton::releasedBackground()
	{
		return _releasedBackground;
	}

	const spk::Panel& PushButton::releasedBackground() const
	{
		return _releasedBackground;
	}

	spk::Panel& PushButton::pressedBackground()
	{
		return _pressedBackground;
	}

	const spk::Panel& PushButton::pressedBackground() const
	{
		return _pressedBackground;
	}

	spk::TextLabel& PushButton::releasedLabel()
	{
		return _releasedLabel;
	}

	const spk::TextLabel& PushButton::releasedLabel() const
	{
		return _releasedLabel;
	}

	spk::TextLabel& PushButton::pressedLabel()
	{
		return _pressedLabel;
	}

	const spk::TextLabel& PushButton::pressedLabel() const
	{
		return _pressedLabel;
	}

	void PushButton::_onMouseMovedEvent(spk::MouseMovedEvent& p_event)
	{
		const bool hovered = absoluteGeometry().contains(p_event.device().position);
		if (_isHovered != hovered)
		{
			_isHovered = hovered;
		}
	}

	void PushButton::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left || absoluteGeometry().contains(p_event.device().position) == false)
		{
			return;
		}

		_isHovered = true;
		_isPressed = true;
		takeFocus(FocusType::Mouse);
		_refreshState();
		p_event.consume();
	}

	void PushButton::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left || _isPressed == false)
		{
			return;
		}

		_isPressed = false;
		_isHovered = absoluteGeometry().contains(p_event.device().position);
		_refreshState();

		if (_isHovered == true)
		{
			_clickProvider.trigger();
			p_event.consume();
		}
	}
}
