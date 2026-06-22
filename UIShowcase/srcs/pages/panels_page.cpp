#include "pages/panels_page.hpp"

#include <array>
#include <string>
#include <utility>

namespace showcase
{
	std::string_view PanelsPage::title() const
	{
		return "Panels / Slice9";
	}

	std::string_view PanelsPage::description() const
	{
		return "spk::Panel draws a nine-slice background carried by a spk::WidgetStyle.";
	}

	void PanelsPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Panels::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Panels::intro", description(), theme::bodyStyle());
		appendLabel(
			*_contentRoot,
			_contentBag,
			"Panels::intro2",
			"The corners keep their size while the edges and center stretch, so a panel scales cleanly.",
			theme::mutedStyle());

		appendHeading(*_contentRoot, _contentBag, "Panels::stylesHeading", "Collection styles");

		const std::array<std::pair<std::string_view, spk::WidgetStyle>, 4> styles = {{
			{"default", spk::WidgetStyle::makeDefault()},
			{"default.light", spk::WidgetStyle::makeDefaultLight()},
			{"default.dark", spk::WidgetStyle::makeDefaultDark()},
			{"default.pressed", spk::WidgetStyle::makeDefaultPressed()},
		}};

		// One row of panels stretched to equal width, with a caption row underneath aligned to
		// the same cells, so each background can be compared at an identical size.
		HorizontalStack &panelRow = appendRow(*_contentRoot, _contentBag, "Panels::row", 120);
		for (const auto &[name, style] : styles)
		{
			auto panel = std::make_unique<spk::Panel>("Panels::panel::" + std::string(name), style, &panelRow);
			adopt(panelRow.layout(), _contentBag, std::move(panel), spk::Layout::SizePolicy::Extend);
		}

		HorizontalStack &captionRow = appendRow(*_contentRoot, _contentBag, "Panels::captionRow", 24);
		for (const auto &[name, style] : styles)
		{
			auto caption = std::make_unique<spk::TextLabel>(
				"Panels::caption::" + std::string(name),
				name,
				theme::mutedStyle(),
				&captionRow);
			caption->setAlignment(spk::HorizontalAlignment::Centered, spk::VerticalAlignment::Centered);
			adopt(captionRow.layout(), _contentBag, std::move(caption), spk::Layout::SizePolicy::Extend);
		}

		appendHeading(*_contentRoot, _contentBag, "Panels::nestedHeading", "Nested / labeled panel");
		{
			// A panel that hosts a label through a LayoutPanel, demonstrating that panels work as
			// background containers, not just standalone decorations.
			auto labeled = std::make_unique<LayoutPanel<spk::VerticalLayout>>(
				"Panels::labeled",
				spk::WidgetStyle::makeDefaultDark(),
				spk::Vector2Int(12, 12),
				_contentRoot.get());
			labeled->activate();

			auto inner = std::make_unique<spk::TextLabel>(
				"Panels::labeled::text",
				"A panel used as a background container for other widgets.",
				theme::bodyStyle(),
				labeled.get());
			inner->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			labeled->layout().addWidget(inner.get(), spk::Layout::SizePolicy::Extend);

			_contentBag.push_back(std::move(inner));
			adopt(_contentRoot->layout(), _contentBag, std::move(labeled), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(0, 60));
		}

		appendSpacer(*_contentRoot, _contentBag, "Panels::spacer");
	}

	void PanelsPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Panels",
			"implemented",
			{
				{"Styles", "default / light / dark / pressed"},
				{"Rendering", "nine-slice corners + stretch"},
				{"Usage", "decoration and container"},
			});
	}
}
