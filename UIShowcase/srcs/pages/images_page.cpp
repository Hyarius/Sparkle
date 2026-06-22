#include "pages/images_page.hpp"

#include <array>
#include <string>

namespace showcase
{
	std::string_view ImagesPage::title() const
	{
		return "Images / Animation";
	}

	std::string_view ImagesPage::description() const
	{
		return "spk::ImageLabel draws a texture section; spk::AnimationLabel cycles a sprite range.";
	}

	void ImagesPage::buildContent(spk::Widget &p_parent)
	{
		_contentRoot = std::make_unique<VerticalStack>("Images::content", &p_parent);
		_contentBag.clear();

		appendLabel(*_contentRoot, _contentBag, "Images::intro", description(), theme::bodyStyle());

		std::shared_ptr<spk::SpriteSheet> iconset =
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet();

		if (iconset == nullptr)
		{
			appendLabel(*_contentRoot, _contentBag, "Images::noIconset", "The default iconset is unavailable.", theme::mutedStyle());
			appendSpacer(*_contentRoot, _contentBag, "Images::spacer");
			return;
		}

		appendLabel(
			*_contentRoot,
			_contentBag,
			"Images::note",
			"This page reuses the default iconset (a 10x10 sprite sheet) as its image source.",
			theme::mutedStyle());

		// ImageLabel: a few static sprites picked out of the iconset by section.
		appendHeading(*_contentRoot, _contentBag, "Images::imageHeading", "ImageLabel");
		HorizontalStack &imageRow = appendRow(*_contentRoot, _contentBag, "Images::imageRow", 64);
		{
			const std::array<std::size_t, 4> spriteIDs = {0, 1, 4, 7};
			for (std::size_t spriteID : spriteIDs)
			{
				auto image = std::make_unique<spk::ImageLabel>("Images::image::" + std::to_string(spriteID), iconset, &imageRow);
				image->setSection(iconset->sprite(spriteID));
				adopt(imageRow.layout(), _contentBag, std::move(image), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(64, 0));
			}
		}

		// AnimationLabel: cycle through the first row of the iconset on a timer.
		appendHeading(*_contentRoot, _contentBag, "Images::animationHeading", "AnimationLabel");
		HorizontalStack &animationRow = appendRow(*_contentRoot, _contentBag, "Images::animationRow", 64);
		{
			auto animation = std::make_unique<spk::AnimationLabel>("Images::animation", iconset, &animationRow);
			animation->setAnimationRange(0, 7);
			animation->setLoopSpeed(spk::Duration(200.0L, spk::TimeUnit::Millisecond));
			adopt(animationRow.layout(), _contentBag, std::move(animation), spk::Layout::SizePolicy::Fixed, spk::Vector2UInt(64, 0));

			auto hint = std::make_unique<spk::TextLabel>(
				"Images::animationHint",
				"Cycles sprites 0 through 7 every 200 ms.",
				theme::mutedStyle(),
				&animationRow);
			hint->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
			adopt(animationRow.layout(), _contentBag, std::move(hint), spk::Layout::SizePolicy::Extend);
		}

		appendSpacer(*_contentRoot, _contentBag, "Images::spacer");
	}

	void ImagesPage::buildProperties(spk::Widget &p_parent)
	{
		buildStatusProperties(
			p_parent,
			_propertiesRoot,
			_propertiesBag,
			"Images",
			"implemented",
			{
				{"ImageLabel", "texture + section"},
				{"AnimationLabel", "sprite range + timer"},
				{"Source", "default iconset"},
			});
	}
}
