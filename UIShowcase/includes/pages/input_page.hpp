#pragma once

#include <memory>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases the editable controls: spk::TextEdit (normal, with placeholder, and obscured) and
	// spk::NumericSpinBox in its integer and floating-point flavors, each with a live readout.
	class InputPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		WidgetBag _contentBag;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		WidgetBag _propertiesBag;

		spk::TextLabel *_echoLabel = nullptr;
		spk::TextLabel *_intLabel = nullptr;
		spk::TextLabel *_floatLabel = nullptr;

		spk::TextEdit::EditionContract _textContract;
		spk::IntSpinBox::EditionContract _intContract;
		spk::FloatSpinBox::EditionContract _floatContract;

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
