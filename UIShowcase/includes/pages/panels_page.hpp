#pragma once

#include <memory>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases spk::Panel and the nine-slice backgrounds carried by spk::WidgetStyle: the
	// default, light, dark and pressed collection styles drawn at a shared size for comparison.
	class PanelsPage : public ShowcasePage
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
