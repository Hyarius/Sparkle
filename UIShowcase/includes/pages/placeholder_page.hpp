#pragma once

#include <memory>
#include <string>
#include <vector>

#include <sparkle.hpp>

#include "showcase_page.hpp"
#include "showcase_widgets.hpp"

namespace showcase
{
	// Generic page used for widgets/features that are planned but not yet implemented.
	//
	// A placeholder is never empty: it states the engineering status and dependencies, so the
	// showcase doubles as a roadmap while the corresponding primitives are being built.
	class PlaceholderPage : public ShowcasePage
	{
	private:
		std::string _title;
		std::string _description;
		std::string _status;
		std::vector<std::string> _dependencies;

		std::unique_ptr<VerticalStack> _contentRoot;
		std::vector<std::unique_ptr<spk::Widget>> _contentWidgets;
		std::unique_ptr<VerticalStack> _propertiesRoot;
		std::vector<std::unique_ptr<spk::Widget>> _propertiesWidgets;

	public:
		PlaceholderPage(
			std::string p_title,
			std::string p_description,
			std::string p_status,
			std::vector<std::string> p_dependencies);

		[[nodiscard]] std::string_view title() const override;
		[[nodiscard]] std::string_view description() const override;

		void buildContent(spk::Widget &p_parent) override;
		void buildProperties(spk::Widget &p_parent) override;
	};
}
