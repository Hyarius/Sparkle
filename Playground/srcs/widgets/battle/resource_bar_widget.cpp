#include "widgets/battle/resource_bar_widget.hpp"

#include <algorithm>

namespace pg
{
	ResourceBarWidget::ResourceBarWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/background", this),
		_fill(p_name + "/fill", this),
		_label(p_name + "/label", "", this)
	{
		_background.useDefaultStyle();
		_fill.useDefaultStyle();
		_label.setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
		activate();
	}

	float ResourceBarWidget::ratio(const ResourceViewModel &p_model) noexcept
	{
		if (p_model.maximum <= 0)
		{
			return 0.0f;
		}
		return std::clamp(static_cast<float>(p_model.current) / static_cast<float>(p_model.maximum), 0.0f, 1.0f);
	}

	void ResourceBarWidget::_layout()
	{
		const spk::Rect2D area = geometry();
		_background.setGeometry({{0, 0}, area.width(), area.height()});
		_fill.setGeometry({{0, 0}, static_cast<std::size_t>(static_cast<float>(area.width()) * ratio(_model)), area.height()});
		_label.setGeometry({{0, 0}, area.width(), area.height()});
	}

	void ResourceBarWidget::_onGeometryChange()
	{
		_layout();
	}

	void ResourceBarWidget::render(const ResourceViewModel &p_model)
	{
		if (_model == p_model)
		{
			return;
		}
		_model = p_model;
		_label.setText(_model.label + " " + std::to_string(_model.current) + "/" + std::to_string(_model.maximum));
		_layout();
	}
}
