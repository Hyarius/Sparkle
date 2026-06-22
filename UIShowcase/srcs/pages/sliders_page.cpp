#include "pages/sliders_page.hpp"

#include <cstdio>
#include <string>

namespace showcase
{
	void SlidersPage::_refreshValueLabel()
	{
		if (_valueLabel == nullptr || _horizontalSlider == nullptr)
		{
			return;
		}

		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "Value: %.1f", _horizontalSlider->value());
		_valueLabel->setText(std::string(buffer));
	}

	std::string_view SlidersPage::title() const
	{
		return "Sliders";
	}

	std::string_view SlidersPage::description() const
	{
		return "spk::SliderBar maps a draggable handle to a value within a range.";
	}

	void SlidersPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Sliders::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Sliders::intro", description(), theme::bodyStyle());

		appendHeading(*_contentRoot, _contentBag, "Sliders::horizontalHeading", "Horizontal");
		HorizontalStack &horizontalRow = appendRow(*_contentRoot, _contentBag, "Sliders::horizontalRow", 28);
		{
			auto slider = std::make_unique<spk::SliderBar>("Sliders::horizontal", spk::Orientation::Horizontal, &horizontalRow);
			slider->setRange(0.0f, 100.0f);
			slider->setValue(35.0f);
			_horizontalSlider = slider.get();
			_horizontalContract = slider->subscribeToEdition([this](float) { _refreshValueLabel(); });
			adopt(horizontalRow.layout(), _contentBag, std::move(slider), spk::Layout::SizePolicy::Extend);

			auto value = std::make_unique<spk::TextLabel>("Sliders::value", "Value: 35.0", theme::accentStyle(), &horizontalRow);
			value->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			_valueLabel = value.get();
			adopt(horizontalRow.layout(), _contentBag, std::move(value), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(120, 0));
		}

		appendHeading(*_contentRoot, _contentBag, "Sliders::verticalHeading", "Vertical");
		HorizontalStack &verticalRow = appendRow(*_contentRoot, _contentBag, "Sliders::verticalRow", 160);
		{
			auto slider = std::make_unique<spk::SliderBar>("Sliders::vertical", spk::Orientation::Vertical, &verticalRow);
			slider->setRange(0.0f, 100.0f);
			slider->setValue(60.0f);
			_verticalContract = slider->subscribeToEdition([this](float) { _refreshValueLabel(); });
			adopt(verticalRow.layout(), _contentBag, std::move(slider), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(28, 0));

			auto hint = std::make_unique<spk::TextLabel>(
				"Sliders::verticalHint",
				"Drag either handle. The horizontal slider's value is shown above; both share the 0-100 range.",
				theme::mutedStyle(),
				&verticalRow);
			hint->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);
			adopt(verticalRow.layout(), _contentBag, std::move(hint), spk::Layout::SizePolicy::Extend);
		}

		appendSpacer(*_contentRoot, _contentBag, "Sliders::spacer");
	}

	void SlidersPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Sliders",
			"implemented",
			{
				{"Orientation", "horizontal + vertical"},
				{"Range", "0 - 100"},
				{"Feedback", "edition contract"},
			});
	}
}
