#pragma once

#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

#include "showcase_page.hpp"
#include "showcase_page_registry.hpp"
#include "showcase_widgets.hpp"

namespace showcase
{
	// Root widget of the showcase application.
	//
	// Owns the persistent shell chrome (top bar, navigation, content mount, properties mount)
	// and swaps the active page in response to navigation. Placement is fully layout-driven:
	// a root vertical layout stacks the top bar (minimum height) over the body (extends), and
	// the body is a horizontal layout of three columns. Each section panel drives its own
	// children, so _onGeometryChange only feeds the root layout and refreshes scroll metrics.
	class ShowcaseRoot : public spk::Widget
	{
	private:
		const ShowcasePageRegistry &_registry;

		// Root: top bar (minimum height) stacked over the body (extends to fill).
		spk::VerticalLayout _rootLayout;
		// Body columns: navigation (minimum width), content (extends), properties (minimum).
		spk::HorizontalLayout _bodyLayout;

		LayoutPanel<spk::HorizontalLayout> _topBar;
		spk::TextLabel _titleLabel;
		spk::TextLabel _metricsLabel;

		LayoutPanel<spk::VerticalLayout> _navPanel;
		spk::ScrollArea<VerticalStack> _navScroll;
		std::vector<std::unique_ptr<spk::PushButton>> _navButtons;
		std::vector<spk::PushButton::Contract> _navContracts;

		LayoutPanel<spk::VerticalLayout> _contentPanel;
		spk::TextLabel _pageTitleLabel;
		FillHost _contentHost;

		LayoutPanel<spk::VerticalLayout> _propertiesPanel;
		spk::TextLabel _propertiesTitleLabel;
		FillHost _propertiesHost;

		std::unique_ptr<ShowcasePage> _activePage;

		void _buildNavigation();
		void _refreshNavMinimalWidth();
		void _showPage(std::size_t p_index);
		void _layoutNavigation();

	protected:
		void _onGeometryChange() override;
		void _onWindowResizedEvent(spk::WindowResizedEvent &p_event) override;

	public:
		ShowcaseRoot(const std::string &p_name, const ShowcasePageRegistry &p_registry, spk::Widget *p_parent = nullptr);
	};
}
