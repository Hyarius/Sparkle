#include "pages/labels_page.hpp"

#include <chrono>
#include <cstdio>
#include <string>

namespace showcase
{
	std::string_view LabelsPage::title() const
	{
		return "Labels / Text";
	}

	std::string_view LabelsPage::description() const
	{
		return "Read-only text widgets: TextLabel, TextArea and DynamicTextLabel.";
	}

	void LabelsPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Labels::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Labels::intro", description(), theme::bodyStyle());

		appendHeading(*_contentRoot, _contentBag, "Labels::textLabelHeading", "TextLabel");
		appendLabel(*_contentRoot, _contentBag, "Labels::body", "Body text uses the primary color.", theme::bodyStyle());
		appendLabel(*_contentRoot, _contentBag, "Labels::muted", "Muted text de-emphasizes secondary information.", theme::mutedStyle());
		appendLabel(*_contentRoot, _contentBag, "Labels::accent", "Accent text highlights an active or important value.", theme::accentStyle());

		// A row of three labels, each aligned differently, sharing the width equally so the
		// alignment behaviour is visible against a common cell width.
		HorizontalStack &alignmentRow = appendRow(*_contentRoot, _contentBag, "Labels::alignmentRow", 28);
		{
			auto makeAligned = [&](const std::string &p_name, std::string_view p_text, spk::HorizontalAlignment p_alignment) {
				auto label = std::make_unique<spk::TextLabel>(p_name, p_text, theme::bodyStyle(), &alignmentRow);
				label->setAlignment(p_alignment, spk::VerticalAlignment::Centered);
				adopt(alignmentRow.layout(), _contentBag, std::move(label), spk::Layout::SizePolicy::Extend);
			};

			makeAligned("Labels::alignLeft", "Left", spk::HorizontalAlignment::Left);
			makeAligned("Labels::alignCenter", "Centered", spk::HorizontalAlignment::Centered);
			makeAligned("Labels::alignRight", "Right", spk::HorizontalAlignment::Right);
		}

		appendHeading(*_contentRoot, _contentBag, "Labels::textAreaHeading", "TextArea");
		{
			auto area = std::make_unique<spk::TextArea>(
				"Labels::textArea",
				"TextArea wraps a paragraph across several lines based on the available width. "
				"It is useful for descriptions, help text and documentation blocks where a single "
				"line would overflow or be clipped by the surrounding layout.",
				theme::bodyStyle(),
				_contentRoot.get());
			area->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top);
			adopt(_contentRoot->layout(), _contentBag, std::move(area), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(0, 90));
		}

		appendHeading(*_contentRoot, _contentBag, "Labels::dynamicHeading", "DynamicTextLabel");
		{
			const auto start = std::chrono::steady_clock::now();
			auto dynamic = std::make_unique<spk::DynamicTextLabel>(
				"Labels::dynamic",
				[start]() -> std::string {
					const auto elapsed = std::chrono::steady_clock::now() - start;
					const auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;
					char buffer[64];
					std::snprintf(buffer, sizeof(buffer), "Refreshed from producer - alive for %.1f s", seconds);
					return std::string(buffer);
				},
				_contentRoot.get());
			dynamic->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			dynamic->setRefreshDuration(spk::Duration(200.0L, spk::TimeUnit::Millisecond));
			adopt(_contentRoot->layout(), _contentBag, std::move(dynamic), spk::Layout::SizePolicy::Minimum);
		}

		appendSpacer(*_contentRoot, _contentBag, "Labels::spacer");
	}

	void LabelsPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Labels",
			"implemented",
			{
				{"TextLabel", "alignment, color, outline"},
				{"TextArea", "word wrapping"},
				{"DynamicTextLabel", "timed producer refresh"},
			});
	}
}
