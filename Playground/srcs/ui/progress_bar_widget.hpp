#pragma once

#include "core/observable_resource.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget.hpp"

#include <string>

namespace pg
{
	class ProgressBarWidget : public spk::Widget
	{
	private:
		spk::Panel _background;
		spk::Panel _fill;
		spk::TextLabel _label;
		const ObservableResource *_integerResource = nullptr;
		const ObservableFloatResource *_floatResource = nullptr;
		ObservableResource::Contract _integerContract;
		ObservableFloatResource::Contract _floatContract;
		std::string _prefix;
		float _ratio = 0.0f;
		float _current = 0.0f;
		float _maximum = 0.0f;
		std::size_t _refreshCount = 0;

		void _refresh(float p_current, float p_maximum);

	protected:
		void _onGeometryChange() override;

	public:
		explicit ProgressBarWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);

		void bind(const ObservableResource &p_resource, std::string p_prefix = {});
		void bind(const ObservableFloatResource &p_resource, std::string p_prefix = {});
		void setValue(float p_current, float p_maximum, std::string p_prefix = {});
		void unbind();

		[[nodiscard]] float ratio() const noexcept;
		[[nodiscard]] float current() const noexcept;
		[[nodiscard]] float maximum() const noexcept;
		[[nodiscard]] std::size_t refreshCount() const noexcept;
		[[nodiscard]] const spk::Font::Text &text() const;
	};
}
