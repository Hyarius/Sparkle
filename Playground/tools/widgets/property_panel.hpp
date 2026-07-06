#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <sparkle.hpp>

namespace pg::tools
{
	class PropertyPanel : public spk::Widget
	{
	private:
		spk::Panel _background;
		spk::FormLayout _layout;
		std::vector<std::unique_ptr<spk::Widget>> _widgets;

		void _onGeometryChange() override;
		spk::TextLabel &_addLabel(const std::string &p_text);

	public:
		using StringCallback = std::function<void(const std::string &)>;
		using IntCallback = std::function<void(int)>;
		using FloatCallback = std::function<void(float)>;
		using BoolCallback = std::function<void(bool)>;

		PropertyPanel(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void clear();
		spk::TextEdit &addString(const std::string &p_label, std::string p_value, StringCallback p_callback);
		spk::SpinBox<int> &addInt(
			const std::string &p_label, int p_value, int p_minimum, int p_maximum, IntCallback p_callback);
		spk::SpinBox<float> &addFloat(
			const std::string &p_label, float p_value, float p_minimum, float p_maximum, FloatCallback p_callback);
		spk::PushButton &addBool(const std::string &p_label, bool p_value, BoolCallback p_callback);
		spk::PushButton &addEnum(
			const std::string &p_label,
			std::vector<std::string> p_values,
			std::string p_value,
			StringCallback p_callback);
		spk::Widget &addCustom(const std::string &p_label, std::unique_ptr<spk::Widget> p_widget);

		// Replaces a polymorphic JSON object with the defaults for the new type while
		// recursively retaining fields that are valid in both representations.
		[[nodiscard]] static nlohmann::json swapPolymorphicType(
			const nlohmann::json &p_current,
			std::string p_type,
			const nlohmann::json &p_defaults);
	};
}
