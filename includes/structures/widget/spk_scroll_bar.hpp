#pragma once

#include <string>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_slider_bar.hpp"
#include "structures/widget/spk_widget.hpp"
#include "type/spk_orientation.hpp"

namespace spk
{
	class ScrollBar : public spk::Widget
	{
	public:
		using Callback = spk::ContractProvider<float>::Callback;
		using Contract = spk::ContractProvider<float>::Contract;

	private:
		spk::ContractProvider<float> _onEditionProvider;

		spk::PushButton _negativeButton;
		spk::PushButton::Contract _negativeButtonContract;
		spk::PushButton _positiveButton;
		spk::PushButton::Contract _positiveButtonContract;
		spk::SliderBar _sliderBar;
		spk::SliderBar::Contract _sliderBarContract;

		float _step = 0.1f;

		void _refreshButtonCaptions();

	protected:
		void _onGeometryChange() override;

	public:
		explicit ScrollBar(const std::string& p_name, spk::Widget* p_parent = nullptr);
		ScrollBar(const std::string& p_name, spk::Orientation p_orientation, spk::Widget* p_parent = nullptr);

		Contract subscribeToEdition(Callback p_callback);

		void setOrientation(spk::Orientation p_orientation);
		void setScale(float p_scale);
		void setRatio(float p_ratio);
		void setStep(float p_step);

		[[nodiscard]] spk::Orientation orientation() const;
		[[nodiscard]] float ratio() const;
		[[nodiscard]] float step() const;

		[[nodiscard]] spk::PushButton& negativeButton();
		[[nodiscard]] const spk::PushButton& negativeButton() const;
		[[nodiscard]] spk::PushButton& positiveButton();
		[[nodiscard]] const spk::PushButton& positiveButton() const;
		[[nodiscard]] spk::SliderBar& sliderBar();
		[[nodiscard]] const spk::SliderBar& sliderBar() const;
	};
}
