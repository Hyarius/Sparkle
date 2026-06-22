#include "showcase_root.hpp"

#include <string>

#include "showcase_theme.hpp"

namespace showcase
{
	namespace
	{
		constexpr int topBarHeight = 46;
		constexpr int propertiesWidth = 320;

		constexpr int navButtonHeight = 30;
		constexpr int navButtonSpacing = 6;
		constexpr int navMargin = 10;
		constexpr int navInset = 6;

		const spk::Vector2Int topBarPadding = {16, 0};
		const spk::Vector2Int sectionPadding = {16, 12};
	}

	ShowcaseRoot::ShowcaseRoot(const std::string &p_name, const ShowcasePageRegistry &p_registry, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_registry(p_registry),
		_topBar(p_name + "::topBar", spk::WidgetStyle::makeDefaultDark(), topBarPadding, this),
		_titleLabel(p_name + "::title", "Sparkle UI Showcase", theme::headingStyle(), &_topBar),
		_metricsLabel(p_name + "::metrics", "Milestone 0 - Scaffold", theme::mutedStyle(), &_topBar),
		_navPanel(p_name + "::nav", spk::WidgetStyle::makeDefaultDark(), {navInset, navInset}, this),
		_navScroll(p_name + "::navScroll", &_navPanel),
		_contentPanel(p_name + "::content", spk::WidgetStyle::makeDefault(), sectionPadding, this),
		_pageTitleLabel(p_name + "::pageTitle", "", theme::headingStyle(), &_contentPanel),
		_contentHost(p_name + "::contentHost", &_contentPanel),
		_propertiesPanel(p_name + "::properties", spk::WidgetStyle::makeDefaultDark(), sectionPadding, this),
		_propertiesTitleLabel(p_name + "::propertiesTitle", "Properties", theme::headingStyle(), &_propertiesPanel),
		_propertiesHost(p_name + "::propertiesHost", &_propertiesPanel)
	{
		_titleLabel.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		_metricsLabel.setAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Centered);
		_pageTitleLabel.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		_propertiesTitleLabel.setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);

		// Top bar: title fills the width, metrics sits at the right.
		_topBar.setMinimalSize(spk::Vector2UInt(0, static_cast<unsigned int>(topBarHeight)));
		_topBar.layout().setElementPadding({16, 0});
		_topBar.layout().addWidget(&_titleLabel, spk::Layout::SizePolicy::Extend);
		_topBar.layout().addWidget(&_metricsLabel, spk::Layout::SizePolicy::Minimum);

		// Navigation: a single, vertically scrollable column filling the panel.
		_navScroll.setScrollBarVisible(spk::Orientation::Horizontal, false);
		_navPanel.layout().addWidget(&_navScroll, spk::Layout::SizePolicy::Extend);

		// Content / properties: a title stacked over the swappable host.
		_contentPanel.layout().setElementPadding({0, 8});
		_contentPanel.layout().addWidget(&_pageTitleLabel, spk::Layout::SizePolicy::Minimum);
		_contentPanel.layout().addWidget(&_contentHost, spk::Layout::SizePolicy::Extend);

		_propertiesPanel.layout().setElementPadding({0, 8});
		_propertiesPanel.layout().addWidget(&_propertiesTitleLabel, spk::Layout::SizePolicy::Minimum);
		_propertiesPanel.layout().addWidget(&_propertiesHost, spk::Layout::SizePolicy::Extend);
		_propertiesPanel.setMinimalSize(spk::Vector2UInt(static_cast<unsigned int>(propertiesWidth), 0));

		_buildNavigation();
		_refreshNavMinimalWidth();

		// Body columns left to right: nav (minimum), content (extends), properties (minimum).
		_bodyLayout.setElementPadding({0, 0});
		_bodyLayout.addWidget(&_navPanel, spk::Layout::SizePolicy::Minimum);
		_bodyLayout.addWidget(&_contentPanel, spk::Layout::SizePolicy::Extend);
		_bodyLayout.addWidget(&_propertiesPanel, spk::Layout::SizePolicy::Minimum);

		// Root: top bar (minimum height) over the body (extends to fill).
		_rootLayout.setElementPadding({0, 0});
		_rootLayout.addWidget(&_topBar, spk::Layout::SizePolicy::Minimum);
		_rootLayout.addLayout(&_bodyLayout, spk::Layout::SizePolicy::Extend);

		activate();

		if (_registry.empty() == false)
		{
			_showPage(0);
		}
	}

	void ShowcaseRoot::_buildNavigation()
	{
		const auto &entries = _registry.entries();

		const spk::WidgetStyle releasedStyle = theme::buttonReleasedStyle();
		const spk::WidgetStyle pressedStyle = theme::buttonPressedStyle();

		VerticalStack &content = _navScroll.contentObject();
		content.setPadding({navMargin, navMargin});
		content.layout().setElementPadding({0, navButtonSpacing});

		for (std::size_t index = 0; index < entries.size(); ++index)
		{
			const ShowcasePageEntry &entry = entries[index];

			auto button = std::make_unique<spk::PushButton>(
				name() + "::nav::" + entry.name,
				entry.name,
				releasedStyle,
				pressedStyle,
				&content);

			// Fixed height per row, full width; the vertical layout stacks them in order.
			spk::Layout::Element *element = content.layout().addWidget(button.get(), spk::Layout::SizePolicy::Fixed);
			element->setSize(spk::Vector2UInt(0, static_cast<unsigned int>(navButtonHeight)));

			_navContracts.push_back(button->subscribeToClick([this, index]() { _showPage(index); }));
			_navButtons.push_back(std::move(button));
		}
	}

	void ShowcaseRoot::_refreshNavMinimalWidth()
	{
		// Size the navigation column to its widest button so no label is clipped, adding the
		// stack margins, the scrollbar gutter and the panel inset. Drives the Minimum policy.
		unsigned int widestButton = 0;
		for (const auto &button : _navButtons)
		{
			widestButton = std::max(widestButton, button->minimalSize().x);
		}

		const int panelWidth =
			static_cast<int>(widestButton) +
			(navMargin * 2) +
			static_cast<int>(_navScroll.scrollBarWidth()) +
			(navInset * 2);

		_navPanel.setMinimalSize(spk::Vector2UInt(static_cast<unsigned int>(std::max(0, panelWidth)), 0));
	}

	void ShowcaseRoot::_showPage(std::size_t p_index)
	{
		const auto &entries = _registry.entries();

		if (p_index >= entries.size())
		{
			return;
		}

		// Destroy the previous page first: this tears down the widgets it parented into the
		// hosts so the hosts no longer reference any of them before the new page is built.
		_activePage.reset();

		_activePage = entries[p_index].factory();

		if (_activePage == nullptr)
		{
			return;
		}

		_pageTitleLabel.setText(_activePage->title());

		_activePage->buildContent(_contentHost);
		_activePage->buildProperties(_propertiesHost);

		// Make sure the hosts are sized, then force the freshly attached page roots to adopt
		// the host geometry (Widget::setGeometry is a no-op when the size is unchanged).
		_onGeometryChange();
		_contentHost.relayout();
		_propertiesHost.relayout();
	}

	void ShowcaseRoot::_layoutNavigation()
	{
		// Drive the scrollable content size; the VerticalStack's layout positions the buttons
		// to fill it. Width matches the view (scrollbar excluded) so there is no horizontal
		// overflow, and the height accounts for the rows, spacing between them and the margins.
		const int viewWidth = static_cast<int>(_navScroll.viewSize().x);
		const int buttonCount = static_cast<int>(_navButtons.size());

		const int contentHeight =
			(buttonCount * navButtonHeight) +
			(std::max(0, buttonCount - 1) * navButtonSpacing) +
			(navMargin * 2);

		_navScroll.setContentSize(spk::Vector2UInt(
			static_cast<unsigned int>(std::max(0, viewWidth)),
			static_cast<unsigned int>(std::max(0, contentHeight))));
	}

	void ShowcaseRoot::_onGeometryChange()
	{
		// Everything is layout-driven: the root layout positions the top bar and the body,
		// the body layout sizes the columns, and each section panel lays out its own children.
		_rootLayout.setGeometry(geometry().atOrigin());

		// The scroll area cannot infer its scrollable extent, so feed it the button stack size.
		_layoutNavigation();
	}

	void ShowcaseRoot::_onWindowResizedEvent(spk::WindowResizedEvent &p_event)
	{
		setGeometry(p_event->rect.atOrigin());
	}
}
