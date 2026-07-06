#include "tools/widgets/property_panel.hpp"

#include <algorithm>
#include <utility>

namespace
{
	void preserveCommon(nlohmann::json &p_target, const nlohmann::json &p_source)
	{
		if (!p_target.is_object() || !p_source.is_object())
		{
			return;
		}
		for (auto &[key, targetValue] : p_target.items())
		{
			if (key == "type" || !p_source.contains(key))
			{
				continue;
			}
			const nlohmann::json &sourceValue = p_source.at(key);
			if (targetValue.is_object() && sourceValue.is_object())
			{
				preserveCommon(targetValue, sourceValue);
			}
			else if (targetValue.type() == sourceValue.type() ||
				(targetValue.is_number() && sourceValue.is_number()))
			{
				targetValue = sourceValue;
			}
		}
	}
}

namespace pg::tools
{
	PropertyPanel::PropertyPanel(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_background(p_name + "/Background", this)
	{
		_background.activate();
		_layout.setElementPadding({6, 6});
		configureMinimalSizeGenerator([this]() {
			return spk::Vector2UInt::max(_layout.minimalSize() + _layout.elementPadding() * 2u, {260u, 0u});
		});
	}

	void PropertyPanel::_onGeometryChange()
	{
		_background.setGeometry(geometry().atOrigin());
		_layout.setGeometry(geometry().atOrigin().shrink(static_cast<spk::Vector2Int>(_layout.elementPadding())));
	}

	spk::TextLabel &PropertyPanel::_addLabel(const std::string &p_text)
	{
		auto label = std::make_unique<spk::TextLabel>(name() + "/Label" + std::to_string(_widgets.size()), p_text, this);
		spk::TextLabel &result = *label;
		label->activate();
		_widgets.push_back(std::move(label));
		return result;
	}

	void PropertyPanel::clear()
	{
		_layout.clear();
		_widgets.clear();
		releaseMinimalSize();
		_onGeometryChange();
	}

	spk::TextEdit &PropertyPanel::addString(
		const std::string &p_label,
		std::string p_value,
		StringCallback p_callback)
	{
		spk::TextLabel &label = _addLabel(p_label);
		auto edit = std::make_unique<spk::TextEdit>(name() + "/String" + std::to_string(_widgets.size()), this);
		spk::TextEdit &result = *edit;
		edit->setText(p_value);
		edit->subscribeToEdition([edit = edit.get(), callback = std::move(p_callback)](const spk::Font::Text &) {
			callback(edit->textAsUTF8());
		}).relinquish();
		edit->activate();
		_widgets.push_back(std::move(edit));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	spk::SpinBox<int> &PropertyPanel::addInt(
		const std::string &p_label,
		int p_value,
		int p_minimum,
		int p_maximum,
		IntCallback p_callback)
	{
		spk::TextLabel &label = _addLabel(p_label);
		auto edit = std::make_unique<spk::SpinBox<int>>(name() + "/Int" + std::to_string(_widgets.size()), this);
		spk::SpinBox<int> &result = *edit;
		edit->setLimits(p_minimum, p_maximum);
		edit->setValue(p_value);
		edit->subscribeToEdition(std::move(p_callback)).relinquish();
		_widgets.push_back(std::move(edit));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	spk::SpinBox<float> &PropertyPanel::addFloat(
		const std::string &p_label,
		float p_value,
		float p_minimum,
		float p_maximum,
		FloatCallback p_callback)
	{
		spk::TextLabel &label = _addLabel(p_label);
		auto edit = std::make_unique<spk::SpinBox<float>>(name() + "/Float" + std::to_string(_widgets.size()), this);
		spk::SpinBox<float> &result = *edit;
		edit->setLimits(p_minimum, p_maximum);
		edit->setStep(0.05f);
		edit->setValue(p_value);
		edit->subscribeToEdition(std::move(p_callback)).relinquish();
		_widgets.push_back(std::move(edit));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	spk::PushButton &PropertyPanel::addBool(
		const std::string &p_label,
		bool p_value,
		BoolCallback p_callback)
	{
		spk::TextLabel &label = _addLabel(p_label);
		auto button = std::make_unique<spk::PushButton>(
			name() + "/Bool" + std::to_string(_widgets.size()), p_value ? "Yes" : "No", this);
		spk::PushButton &result = *button;
		button->subscribeToClick([button = button.get(), value = p_value, callback = std::move(p_callback)]() mutable {
			value = !value;
			button->setText(value ? "Yes" : "No");
			callback(value);
		}).relinquish();
		button->activate();
		_widgets.push_back(std::move(button));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	spk::PushButton &PropertyPanel::addEnum(
		const std::string &p_label,
		std::vector<std::string> p_values,
		std::string p_value,
		StringCallback p_callback)
	{
		spk::TextLabel &label = _addLabel(p_label);
		auto button = std::make_unique<spk::PushButton>(
			name() + "/Enum" + std::to_string(_widgets.size()), p_value, this);
		spk::PushButton &result = *button;
		auto current = std::ranges::find(p_values, p_value);
		std::size_t index = current == p_values.end() ? 0u : static_cast<std::size_t>(std::distance(p_values.begin(), current));
		button->subscribeToClick([
			button = button.get(), values = std::move(p_values), index, callback = std::move(p_callback)]() mutable {
			if (values.empty())
			{
				return;
			}
			index = (index + 1) % values.size();
			button->setText(values[index]);
			callback(values[index]);
		}).relinquish();
		button->activate();
		_widgets.push_back(std::move(button));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	spk::Widget &PropertyPanel::addCustom(const std::string &p_label, std::unique_ptr<spk::Widget> p_widget)
	{
		spk::TextLabel &label = _addLabel(p_label);
		spk::Widget &result = *p_widget;
		p_widget->activate();
		_widgets.push_back(std::move(p_widget));
		_layout.addRow(&label, &result);
		releaseMinimalSize();
		return result;
	}

	nlohmann::json PropertyPanel::swapPolymorphicType(
		const nlohmann::json &p_current,
		std::string p_type,
		const nlohmann::json &p_defaults)
	{
		nlohmann::json result = p_defaults;
		if (!result.is_object())
		{
			result = nlohmann::json::object();
		}
		preserveCommon(result, p_current);
		result["type"] = std::move(p_type);
		return result;
	}
}
