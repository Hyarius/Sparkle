#include "structures/widget/spk_icon_button.hpp"

#include <stdexcept>
#include <utility>

#include "structures/widget/spk_widget_style.hpp"

namespace spk
{
	IconButton::IconButton(const std::string& p_name, spk::Widget* p_parent) :
		IconButton(
			p_name,
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).iconSpriteSheet(),
			0,
			p_parent)
	{
	}

	IconButton::IconButton(
		const std::string& p_name,
		std::shared_ptr<spk::SpriteSheet> p_iconset,
		size_t p_iconSpriteID,
		spk::Widget* p_parent) :
		spk::PushButton(p_name, p_parent),
		_iconset(std::move(p_iconset)),
		_iconSpriteID(p_iconSpriteID)
	{
		if (_iconset == nullptr)
		{
			throw std::invalid_argument("IconButton iconset cannot be null");
		}

		_refreshIcon();
	}

	void IconButton::_refreshIcon()
	{
		setIcon(_iconset, _iconSpriteID);
	}

	void IconButton::applyStyle(const spk::WidgetStyle& p_style)
	{
		spk::PushButton::applyStyle(p_style);

		if (p_style.iconSpriteSheet() != nullptr)
		{
			_iconset = p_style.iconSpriteSheet();
			_refreshIcon();
		}
	}

	void IconButton::setIconset(std::shared_ptr<spk::SpriteSheet> p_iconset)
	{
		if (p_iconset == nullptr)
		{
			throw std::invalid_argument("IconButton iconset cannot be null");
		}

		_iconset = std::move(p_iconset);
		_refreshIcon();
	}

	void IconButton::setIconSpriteID(size_t p_spriteID)
	{
		_iconSpriteID = p_spriteID;
		_refreshIcon();
	}

	void IconButton::setIconSpriteID(const spk::Vector2UInt& p_spriteCoordinates)
	{
		setIconSpriteID(_iconset->spriteID(p_spriteCoordinates));
	}

	const std::shared_ptr<spk::SpriteSheet>& IconButton::iconset() const
	{
		return _iconset;
	}

	size_t IconButton::iconSpriteID() const
	{
		return _iconSpriteID;
	}
}
