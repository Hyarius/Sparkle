#pragma once

#include <memory>
#include <vector>

#include <sparkle.hpp>

#include "showcase_page.hpp"
#include "showcase_widgets.hpp"

namespace showcase
{
	// Landing page of the showcase. Explains what the tool is for and lists the milestones,
	// so that opening the application immediately communicates intent and current state.
	class OverviewPage : public ShowcasePage
	{
	private:
		std::unique_ptr<VerticalStack> _contentRoot;
		std::vector<std::unique_ptr<spk::Widget>> _contentWidgets;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		std::vector<std::unique_ptr<spk::Widget>> _propertiesWidgets;

	public:
		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
