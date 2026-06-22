#pragma once

#include <memory>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases spk::SliderBar in both orientations, with a live value readout driven by the
	// edition contract and a configurable range.
	class SlidersPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		WidgetBag _contentBag;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		WidgetBag _propertiesBag;

		spk::SliderBar *_horizontalSlider = nullptr;
		spk::TextLabel *_valueLabel = nullptr;
		spk::SliderBar::Contract _horizontalContract;
		spk::SliderBar::Contract _verticalContract;

		void _refreshValueLabel();

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
