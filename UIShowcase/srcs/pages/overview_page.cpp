#include "pages/overview_page.hpp"

#include <array>
#include <string>
#include <utility>

#include "showcase_theme.hpp"

namespace showcase
{
	namespace
	{
		spk::TextLabel &appendLabel(
			VerticalStack &p_stack,
			std::vector<std::unique_ptr<spk::Widget>> &p_storage,
			const std::string &p_name,
			std::string_view p_text,
			const spk::WidgetStyle &p_style)
		{
			auto label = std::make_unique<spk::TextLabel>(p_name, p_text, p_style, &p_stack);
			label->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);

			spk::TextLabel &reference = *label;
			p_stack.layout().addWidget(label.get(), spk::Layout::SizePolicy::Minimum);
			p_storage.push_back(std::move(label));
			return reference;
		}

		void appendStretchSpacer(
			VerticalStack &p_stack,
			std::vector<std::unique_ptr<spk::Widget>> &p_storage,
			const std::string &p_name)
		{
			auto spacer = std::make_unique<spk::SpacerWidget>(p_name, &p_stack);
			p_stack.layout().addWidget(spacer.get(), spk::Layout::SizePolicy::Extend);
			p_storage.push_back(std::move(spacer));
		}
	}

	std::string_view OverviewPage::title() const
	{
		return "Overview";
	}

	std::string_view OverviewPage::description() const
	{
		return "Interactive gallery and validation dashboard for the Sparkle widget library.";
	}

	void OverviewPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Overview::content", &p_parent);
		_contentWidgets.clear();

		appendLabel(*_contentRoot, _contentWidgets, "Overview::intro", description(), theme::bodyStyle());
		appendLabel(
			*_contentRoot,
			_contentWidgets,
			"Overview::intro2",
			"Each page documents a widget or layout behavior and exposes live state where useful.",
			theme::bodyStyle());

		appendLabel(*_contentRoot, _contentWidgets, "Overview::milestonesHeading", "Milestones", theme::accentStyle());

		static constexpr std::array<std::string_view, 6> milestones = {
			"  0  Scaffold - shell, navigation, page registry (current)",
			"  1  Existing widget gallery",
			"  2  Layout validation (linear, grid, form, layout lab)",
			"  3  Diagnostics (events, clipping, theme, metrics)",
			"  4  Overlay infrastructure, then tooltips",
			"  5+ Missing primitive widgets (progress, checkbox, tabs, combo)"};

		std::size_t index = 0;
		for (std::string_view milestone : milestones)
		{
			appendLabel(*_contentRoot, _contentWidgets, "Overview::milestone::" + std::to_string(index), milestone, theme::bodyStyle());
			++index;
		}

		appendStretchSpacer(*_contentRoot, _contentWidgets, "Overview::spacer");
	}

	void OverviewPage::buildProperties(spk::Widget &p_parent)
	{
		_propertiesRoot = std::make_unique<VerticalStack>("Overview::propertiesRoot", &p_parent);
		_propertiesWidgets.clear();

		appendLabel(*_propertiesRoot, _propertiesWidgets, "Overview::prop::status", "Status", theme::mutedStyle());
		appendLabel(*_propertiesRoot, _propertiesWidgets, "Overview::prop::statusValue", "implemented", theme::bodyStyle());
		appendLabel(*_propertiesRoot, _propertiesWidgets, "Overview::prop::shell", "Shell", theme::mutedStyle());
		appendLabel(*_propertiesRoot, _propertiesWidgets, "Overview::prop::shellValue", "top bar / nav / content / properties", theme::bodyStyle());

		appendStretchSpacer(*_propertiesRoot, _propertiesWidgets, "Overview::propertiesSpacer");
	}
}
