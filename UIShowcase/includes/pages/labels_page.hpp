#pragma once

#include <memory>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases the read-only text widgets: TextLabel (alignment, color, outline), TextArea
	// (multi-line word wrapping) and DynamicTextLabel (text refreshed from a producer).
	class LabelsPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		WidgetBag _contentBag;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		WidgetBag _propertiesBag;

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
