#pragma once

#include <string_view>

#include <sparkle.hpp>

namespace showcase
{
	// Abstraction for a single showcase page.
	//
	// A page builds its own widget tree into the parents handed to it. The page must own
	// every widget it creates so that destroying the page tears the widget subtree down,
	// which is how ShowcaseRoot swaps pages when the user navigates.
	class ShowcasePage
	{
	public:
		virtual ~ShowcasePage() = default;

		[[nodiscard]] virtual std::string_view title() const = 0;
		[[nodiscard]] virtual std::string_view description() const = 0;

		// Build the main content of the page. p_parent is a fresh mount point that stretches
		// a single root widget to fill the available content area.
		virtual void buildContent(spk::Widget &p_parent) = 0;

		// Build the right-hand properties panel for the page. May be left empty.
		virtual void buildProperties(spk::Widget &p_parent) = 0;
	};
}
