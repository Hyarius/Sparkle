#pragma once

#include <memory>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases spk::ImageLabel (a static texture/section) and spk::AnimationLabel (a sprite
	// range cycled on a timer). Both reuse the default iconset so the page needs no extra assets.
	class ImagesPage : public ShowcasePage
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
