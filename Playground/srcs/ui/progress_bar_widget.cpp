#include "ui/progress_bar_widget.hpp"

#include "structures/widget/spk_widget_style.hpp"

#include <algorithm>
#include <cstdio>
#include <utility>

namespace pg
{
	ProgressBarWidget::ProgressBarWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this),
		_fill(p_name + "/Fill", spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultSliderBody), this),
		_label(p_name + "/Label", this)
	{
		_label.setTextSize(spk::Font::Size(13, 1));
	}

	void ProgressBarWidget::_refresh(float p_current, float p_maximum)
	{
		_current = std::max(0.0f, p_current);
		_maximum = std::max(0.0f, p_maximum);
		_ratio = _maximum > 0.0f ? std::clamp(_current / _maximum, 0.0f, 1.0f) : 0.0f;
		char values[64];
		if (_integerResource != nullptr)
		{
			std::snprintf(values, sizeof(values), "%d/%d", static_cast<int>(_current), static_cast<int>(_maximum));
		}
		else
		{
			std::snprintf(values, sizeof(values), "%.1f/%.1f", _current, _maximum);
		}
		_label.setText(_prefix.empty() ? values : _prefix + " " + values);
		++_refreshCount;
		_onGeometryChange();
	}

	void ProgressBarWidget::bind(const ObservableResource &p_resource, std::string p_prefix)
	{
		unbind();
		_integerResource = &p_resource;
		_prefix = std::move(p_prefix);
		_integerContract = const_cast<ObservableResource &>(p_resource).subscribe([this](const ObservableResource &p_value) {
			_refresh(static_cast<float>(p_value.current()), static_cast<float>(p_value.max()));
		});
		_refresh(static_cast<float>(p_resource.current()), static_cast<float>(p_resource.max()));
	}

	void ProgressBarWidget::bind(const ObservableFloatResource &p_resource, std::string p_prefix)
	{
		unbind();
		_floatResource = &p_resource;
		_prefix = std::move(p_prefix);
		_floatContract = const_cast<ObservableFloatResource &>(p_resource).subscribe([this](const ObservableFloatResource &p_value) {
			_refresh(p_value.current(), p_value.max());
		});
		_refresh(p_resource.current(), p_resource.max());
	}

	void ProgressBarWidget::setValue(float p_current, float p_maximum, std::string p_prefix)
	{
		unbind();
		_prefix = std::move(p_prefix);
		_refresh(p_current, p_maximum);
	}

	void ProgressBarWidget::unbind()
	{
		_integerContract.resign();
		_floatContract.resign();
		_integerResource = nullptr;
		_floatResource = nullptr;
		_prefix.clear();
	}

	void ProgressBarWidget::_onGeometryChange()
	{
		const spk::Rect2D bounds(0, 0, geometry().width(), geometry().height());
		_background.setGeometry(bounds);
		_fill.setGeometry(spk::Rect2D(0, 0, static_cast<unsigned int>(static_cast<float>(geometry().width()) * _ratio), geometry().height()));
		_label.setGeometry(bounds);
	}

	float ProgressBarWidget::ratio() const noexcept { return _ratio; }
	float ProgressBarWidget::current() const noexcept { return _current; }
	float ProgressBarWidget::maximum() const noexcept { return _maximum; }
	std::size_t ProgressBarWidget::refreshCount() const noexcept { return _refreshCount; }
	const spk::Font::Text &ProgressBarWidget::text() const { return _label.text(); }
}
