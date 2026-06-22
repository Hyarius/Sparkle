#pragma once

#include <memory>
#include <vector>

#include "page_support.hpp"
#include "showcase_page.hpp"

namespace showcase
{
	// Showcases the clickable controls: PushButton (text + click feedback), IconButton and
	// CheckableIconButton (iconset-backed), and CommandPanel (a right-aligned button group).
	class ButtonsPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		WidgetBag _contentBag;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		WidgetBag _propertiesBag;

		// Live feedback labels owned by _contentBag; kept as raw pointers so the click and toggle
		// callbacks can update them in place.
		spk::TextLabel *_clickStatus = nullptr;
		int _clickCount = 0;
		spk::TextLabel *_checkStatus = nullptr;

		std::vector<spk::PushButton::Contract> _clickContracts;
		spk::CheckableIconButton::StateContract _checkContract;

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
