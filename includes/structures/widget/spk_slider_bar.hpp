#pragma once

#include <string>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_orientation.hpp"

namespace spk
{
	class SliderBar : public spk::Widget
	{
	public:
		using Callback = spk::ContractProvider<float>::Callback;
		using Contract = spk::ContractProvider<float>::Contract;

	private:
		spk::Panel _background;
		spk::Panel _body;

		spk::ContractProvider<float> _onEditionProvider;

		spk::Orientation _orientation = spk::Orientation::Horizontal;

		bool _isDragged = false;
		spk::Vector2Int _draggedMousePosition;
		float _draggedRatio = 0.0f;
		float _scale = 0.1f;
		float _ratio = 0.0f;
		float _minValue = 0.0f;
		float _maxValue = 100.0f;

		void _refreshBodyGeometry();
		[[nodiscard]] float _ratioFromPosition(const spk::Vector2Int &p_position) const;

	protected:
		void _onGeometryChange() override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;

	public:
		explicit SliderBar(const std::string &p_name, spk::Widget *p_parent = nullptr);
		SliderBar(
			const std::string &p_name,
			spk::Orientation p_orientation,
			spk::Widget *p_parent = nullptr);

		void applyStyle(const spk::WidgetStyle &p_style) override;

		Contract subscribeToEdition(Callback p_callback);

		void setOrientation(spk::Orientation p_orientation);
		void setScale(float p_scale);
		void setRange(float p_minValue, float p_maxValue);
		void setRatio(float p_ratio);
		void setValue(float p_value);

		[[nodiscard]] spk::Orientation orientation() const;
		[[nodiscard]] float scale() const;
		[[nodiscard]] float ratio() const;
		[[nodiscard]] float value() const;
		[[nodiscard]] float minValue() const;
		[[nodiscard]] float maxValue() const;
		[[nodiscard]] bool isDragged() const;

		[[nodiscard]] spk::Panel &background();
		[[nodiscard]] const spk::Panel &background() const;
		[[nodiscard]] spk::Panel &body();
		[[nodiscard]] const spk::Panel &body() const;
	};
}
