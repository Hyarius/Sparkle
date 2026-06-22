#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <sparkle.hpp>

#include "showcase_theme.hpp"
#include "showcase_widgets.hpp"

namespace showcase
{
	// Owns every widget a page builds. A page keeps one bag per mount point so destroying the
	// page (and the bag) tears the whole widget subtree down in one step. The order in which the
	// bag releases its widgets does not matter: Sparkle's hierarchy detaches parents and children
	// from each other on destruction, so either teardown order is safe.
	using WidgetBag = std::vector<std::unique_ptr<spk::Widget>>;

	// Take ownership of p_widget, attach it to p_layout with the given policy and an optional
	// requested size, and return a typed reference for further configuration. The size is only
	// honoured by the Fixed/Extend policies; it is harmless for the others.
	template <typename TWidget>
	TWidget &adopt(
		spk::Layout &p_layout,
		WidgetBag &p_bag,
		std::unique_ptr<TWidget> p_widget,
		spk::Layout::SizePolicy p_policy,
		const spk::Vector2UInt &p_size = {0, 0})
	{
		TWidget &reference = *p_widget;

		spk::Layout::Element *element = p_layout.addWidget(p_widget.get(), p_policy);
		element->setSize(p_size);

		p_bag.push_back(std::move(p_widget));
		return reference;
	}

	// Append a left-aligned text label sized to its content (Minimum policy).
	inline spk::TextLabel &appendLabel(
		VerticalStack &p_stack,
		WidgetBag &p_bag,
		const std::string &p_name,
		std::string_view p_text,
		const spk::WidgetStyle &p_style)
	{
		auto label = std::make_unique<spk::TextLabel>(p_name, p_text, p_style, &p_stack);
		label->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		return adopt(p_stack.layout(), p_bag, std::move(label), spk::Layout::SizePolicy::Minimum);
	}

	// Append a section heading using the accent style.
	inline spk::TextLabel &appendHeading(
		VerticalStack &p_stack,
		WidgetBag &p_bag,
		const std::string &p_name,
		std::string_view p_text)
	{
		return appendLabel(p_stack, p_bag, p_name, p_text, theme::accentStyle());
	}

	// Append an invisible spacer that absorbs the remaining vertical space, so the widgets above
	// keep their natural height instead of being stretched across the whole stack.
	inline void appendSpacer(VerticalStack &p_stack, WidgetBag &p_bag, const std::string &p_name)
	{
		auto spacer = std::make_unique<spk::SpacerWidget>(p_name, &p_stack);
		adopt(p_stack.layout(), p_bag, std::move(spacer), spk::Layout::SizePolicy::Extend);
	}

	// Append a fixed-height horizontal row to the stack and return it so the caller can populate
	// its horizontal layout. The row is owned by the bag like any other widget.
	inline HorizontalStack &appendRow(
		VerticalStack &p_stack,
		WidgetBag &p_bag,
		const std::string &p_name,
		unsigned int p_height)
	{
		auto row = std::make_unique<HorizontalStack>(p_name, &p_stack);
		return adopt(
			p_stack.layout(),
			p_bag,
			std::move(row),
			spk::Layout::SizePolicy::Fixed,
			spk::Vector2UInt(0, p_height));
	}

	// Build a small "Status / ..." block into a properties mount point. Pages call this from
	// buildProperties so every page exposes a consistent engineering status panel.
	inline void buildStatusProperties(
		spk::Widget &p_parent,
		std::unique_ptr<VerticalStack> &p_root,
		WidgetBag &p_bag,
		const std::string &p_prefix,
		std::string_view p_status,
		const std::vector<std::pair<std::string_view, std::string_view>> &p_fields)
	{
		p_root = std::make_unique<VerticalStack>(p_prefix + "::propertiesRoot", &p_parent);
		p_bag.clear();

		appendLabel(*p_root, p_bag, p_prefix + "::prop::statusKey", "Status", theme::mutedStyle());
		appendLabel(*p_root, p_bag, p_prefix + "::prop::statusValue", p_status, theme::bodyStyle());

		std::size_t index = 0;
		for (const auto &[key, value] : p_fields)
		{
			const std::string suffix = std::to_string(index);
			appendLabel(*p_root, p_bag, p_prefix + "::prop::key" + suffix, key, theme::mutedStyle());
			appendLabel(*p_root, p_bag, p_prefix + "::prop::value" + suffix, value, theme::bodyStyle());
			++index;
		}

		appendSpacer(*p_root, p_bag, p_prefix + "::propertiesSpacer");
	}
}
