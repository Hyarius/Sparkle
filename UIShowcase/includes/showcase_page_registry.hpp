#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "showcase_page.hpp"

namespace showcase
{
	// A single entry in the navigation: a category group, a display name, and a factory that
	// lazily instantiates the page when the user selects it.
	struct ShowcasePageEntry
	{
		std::string category;
		std::string name;
		std::function<std::unique_ptr<ShowcasePage>()> factory;
	};

	// Ordered collection of page entries. Pages are registered up front; ShowcaseRoot reads
	// the entries to build the navigation and lazily creates a page through its factory.
	class ShowcasePageRegistry
	{
	private:
		std::vector<ShowcasePageEntry> _entries;

	public:
		void add(std::string p_category, std::string p_name, std::function<std::unique_ptr<ShowcasePage>()> p_factory);

		[[nodiscard]] const std::vector<ShowcasePageEntry> &entries() const;
		[[nodiscard]] bool empty() const;
	};

	// Registers the milestone 0 set of pages: the real overview page plus placeholder pages
	// for everything still on the roadmap, so the shell is navigable from the first build.
	void registerDefaultPages(ShowcasePageRegistry &p_registry);
}
