#include "structures/widget/spk_slider_bar.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace spk
{
	namespace
	{
		[[nodiscard]] unsigned int scaledLength(unsigned int p_totalLength, float p_scale)
		{
			return static_cast<unsigned int>(static_cast<float>(p_totalLength) * p_scale);
		}

		[[nodiscard]] unsigned int effectiveBodyLength(
			unsigned int p_totalLength,
			unsigned int p_crossLength,
			float p_scale)
		{
			return std::min(p_totalLength, std::max(scaledLength(p_totalLength, p_scale), p_crossLength));
		}
	}

	SliderBar::SliderBar(const std::string &p_name, spk::Widget *p_parent) :
		SliderBar(p_name, spk::Orientation::Horizontal, p_parent)
	{
	}

	SliderBar::SliderBar(
		const std::string &p_name,
		spk::Orientation p_orientation,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(
			p_name + "::background",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			this),
		_body(
			p_name + "::body",
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultSliderBody),
			this),
		_orientation(p_orientation)
	{
		activate();
	}

	void SliderBar::applyStyle(const spk::WidgetStyle &p_style)
	{
		_background.applyStyle(p_style);
	}

	SliderBar::Contract SliderBar::subscribeToEdition(Callback p_callback)
	{
		return _onEditionProvider.subscribe(std::move(p_callback));
	}

	void SliderBar::_refreshBodyGeometry()
	{
		spk::Vector2UInt bodySize;
		spk::Vector2Int bodyAnchor;

		switch (_orientation)
		{
		case spk::Orientation::Horizontal:
		{
			const unsigned int bodyLength = effectiveBodyLength(geometry().width(), geometry().height(), _scale);
			const unsigned int range = geometry().width() - bodyLength;

			bodySize = {
				bodyLength,
				geometry().height()};
			bodyAnchor = {
				static_cast<int>(static_cast<float>(range) * _ratio),
				0};
			break;
		}
		case spk::Orientation::Vertical:
		{
			const unsigned int bodyLength = effectiveBodyLength(geometry().height(), geometry().width(), _scale);
			const unsigned int range = geometry().height() - bodyLength;

			bodySize = {
				geometry().width(),
				bodyLength};
			bodyAnchor = {
				0,
				static_cast<int>(static_cast<float>(range) * _ratio)};
			break;
		}
		}

		_body.setGeometry(spk::Rect2D(bodyAnchor, bodySize));
	}

	void SliderBar::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_refreshBodyGeometry();
	}

	float SliderBar::_ratioFromPosition(const spk::Vector2Int &p_position) const
	{
		const bool horizontal = (_orientation == spk::Orientation::Horizontal);

		const unsigned int bodyLength =
			horizontal
				? effectiveBodyLength(geometry().width(), geometry().height(), _scale)
				: effectiveBodyLength(geometry().height(), geometry().width(), _scale);
		const float range =
			horizontal
				? static_cast<float>(geometry().width() - bodyLength)
				: static_cast<float>(geometry().height() - bodyLength);

		if (range <= 0.0f)
		{
			return _ratio;
		}

		const float localPosition =
			horizontal
				? static_cast<float>(p_position.x - absoluteGeometry().left())
				: static_cast<float>(p_position.y - absoluteGeometry().top());

		return std::clamp((localPosition - static_cast<float>(bodyLength) / 2.0f) / range, 0.0f, 1.0f);
	}

	void SliderBar::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left)
		{
			return;
		}

		if (_body.viewport().geometry().contains(p_event.device().position) == false)
		{
			if (absoluteGeometry().contains(p_event.device().position) == false)
			{
				return;
			}

			setRatio(_ratioFromPosition(p_event.device().position));
		}

		_isDragged = true;
		_draggedMousePosition = p_event.device().position;
		_draggedRatio = _ratio;
		takeFocus(FocusType::Mouse);
		p_event.consume();
	}

	void SliderBar::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || _isDragged == false)
		{
			return;
		}

		_isDragged = false;
		releaseFocus(FocusType::Mouse);
		p_event.consume();
	}

	void SliderBar::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		if (_isDragged == false)
		{
			return;
		}

		const unsigned int bodyLength =
			(_orientation == spk::Orientation::Horizontal)
				? effectiveBodyLength(geometry().width(), geometry().height(), _scale)
				: effectiveBodyLength(geometry().height(), geometry().width(), _scale);
		const float range =
			(_orientation == spk::Orientation::Horizontal)
				? static_cast<float>(geometry().width() - bodyLength)
				: static_cast<float>(geometry().height() - bodyLength);

		if (range <= 0.0f)
		{
			return;
		}

		const float mouseDelta =
			(_orientation == spk::Orientation::Horizontal)
				? static_cast<float>(p_event.device().position.x - _draggedMousePosition.x)
				: static_cast<float>(p_event.device().position.y - _draggedMousePosition.y);

		const float newRatio = std::clamp(_draggedRatio + (mouseDelta / range), 0.0f, 1.0f);
		if (newRatio == _ratio)
		{
			return;
		}

		_ratio = newRatio;
		_refreshBodyGeometry();
		_onEditionProvider.trigger(_ratio);
		p_event.consume();
	}

	void SliderBar::setOrientation(spk::Orientation p_orientation)
	{
		if (_orientation == p_orientation)
		{
			return;
		}

		_orientation = p_orientation;
		_refreshBodyGeometry();
	}

	void SliderBar::setScale(float p_scale)
	{
		if (p_scale <= 0.0f || p_scale > 1.0f)
		{
			throw std::invalid_argument("SliderBar scale must belong to ]0, 1]");
		}

		_scale = p_scale;
		_refreshBodyGeometry();
	}

	void SliderBar::setRange(float p_minValue, float p_maxValue)
	{
		if (p_maxValue < p_minValue)
		{
			throw std::invalid_argument("SliderBar maximum value cannot be lower than its minimum value");
		}

		_minValue = p_minValue;
		_maxValue = p_maxValue;
	}

	void SliderBar::setRatio(float p_ratio)
	{
		const float newRatio = std::clamp(p_ratio, 0.0f, 1.0f);
		if (newRatio == _ratio)
		{
			return;
		}

		_ratio = newRatio;
		_refreshBodyGeometry();
		_onEditionProvider.trigger(_ratio);
	}

	void SliderBar::setValue(float p_value)
	{
		if (_maxValue == _minValue)
		{
			setRatio(0.0f);
			return;
		}

		setRatio((p_value - _minValue) / (_maxValue - _minValue));
	}

	spk::Orientation SliderBar::orientation() const
	{
		return _orientation;
	}

	float SliderBar::scale() const
	{
		return _scale;
	}

	float SliderBar::ratio() const
	{
		return _ratio;
	}

	float SliderBar::value() const
	{
		return std::lerp(_minValue, _maxValue, _ratio);
	}

	float SliderBar::minValue() const
	{
		return _minValue;
	}

	float SliderBar::maxValue() const
	{
		return _maxValue;
	}

	bool SliderBar::isDragged() const
	{
		return _isDragged;
	}

	spk::Panel &SliderBar::background()
	{
		return _background;
	}

	const spk::Panel &SliderBar::background() const
	{
		return _background;
	}

	spk::Panel &SliderBar::body()
	{
		return _body;
	}

	const spk::Panel &SliderBar::body() const
	{
		return _body;
	}
}
