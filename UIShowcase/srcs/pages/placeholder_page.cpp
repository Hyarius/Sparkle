#include "pages/placeholder_page.hpp"

#include <utility>

#include "showcase_theme.hpp"

namespace showcase
{
	namespace
	{
		// Appends a left-aligned text label to a vertical stack and retains ownership of it.
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

		// Appends an invisible spacer that absorbs the remaining vertical space so the labels
		// above keep their natural height instead of being stretched to fill the stack.
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

	PlaceholderPage::PlaceholderPage(
		std::string p_title,
		std::string p_description,
		std::string p_status,
		std::vector<std::string> p_dependencies) :
		_title(std::move(p_title)),
		_description(std::move(p_description)),
		_status(std::move(p_status)),
		_dependencies(std::move(p_dependencies))
	{
	}

	std::string_view PlaceholderPage::title() const
	{
		return _title;
	}

	std::string_view PlaceholderPage::description() const
	{
		return _description;
	}

	void PlaceholderPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>(_title + "::content", &p_parent);
		_contentWidgets.clear();

		appendLabel(*_contentRoot, _contentWidgets, _title + "::status", "Status: " + _status, theme::accentStyle());
		appendLabel(*_contentRoot, _contentWidgets, _title + "::description", _description, theme::bodyStyle());

		appendLabel(*_contentRoot, _contentWidgets, _title + "::dependsHeading", "Depends on:", theme::mutedStyle());

		if (_dependencies.empty() == true)
		{
			appendLabel(*_contentRoot, _contentWidgets, _title + "::dependsNone", "  - nothing (ready to implement)", theme::bodyStyle());
		}
		else
		{
			std::size_t index = 0;
			for (const std::string &dependency : _dependencies)
			{
				appendLabel(
					*_contentRoot,
					_contentWidgets,
					_title + "::depends::" + std::to_string(index),
					"  - " + dependency,
					theme::bodyStyle());
				++index;
			}
		}

		appendStretchSpacer(*_contentRoot, _contentWidgets, _title + "::spacer");
	}

	void PlaceholderPage::buildProperties(spk::Widget &p_parent)
	{
		_propertiesRoot = std::make_unique<VerticalStack>(_title + "::propertiesRoot", &p_parent);
		_propertiesWidgets.clear();

		appendLabel(*_propertiesRoot, _propertiesWidgets, _title + "::prop::status", "Status", theme::mutedStyle());
		appendLabel(*_propertiesRoot, _propertiesWidgets, _title + "::prop::statusValue", _status, theme::bodyStyle());
		appendLabel(*_propertiesRoot, _propertiesWidgets, _title + "::prop::deps", "Blocking dependencies", theme::mutedStyle());
		appendLabel(
			*_propertiesRoot,
			_propertiesWidgets,
			_title + "::prop::depsValue",
			std::to_string(_dependencies.size()),
			theme::bodyStyle());

		appendStretchSpacer(*_propertiesRoot, _propertiesWidgets, _title + "::propertiesSpacer");
	}
}
