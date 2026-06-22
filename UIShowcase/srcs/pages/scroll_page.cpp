#include "pages/scroll_page.hpp"

namespace showcase
{
	std::string_view ScrollPage::title() const
	{
		return "Scroll";
	}

	std::string_view ScrollPage::description() const
	{
		return "spk::ScrollArea clips a viewport over content larger than the available space.";
	}

	void ScrollPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Scroll::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Scroll::intro", description(), theme::bodyStyle());
		appendLabel(
			*_contentRoot,
			_contentBag,
			"Scroll::hint",
			"Drag the scroll bar or use the mouse wheel over the list below.",
			theme::mutedStyle());

		auto host = std::make_unique<ScrollHost>("Scroll::host", _contentRoot.get());
		adopt(_contentRoot->layout(), _contentBag, std::move(host), spk::Layout::SizePolicy::Extend);
	}

	void ScrollPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Scroll",
			"implemented",
			{
				{"ScrollArea", "clipped viewport"},
				{"Scroll bar", "vertical"},
				{"Input", "mouse wheel"},
			});
	}
}
