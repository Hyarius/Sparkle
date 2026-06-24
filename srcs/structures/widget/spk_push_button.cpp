#include "structures/widget/spk_push_button.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	namespace
	{
		[[nodiscard]] spk::Vector2UInt iconMinimalSize(
			const spk::ImageLabel &p_icon,
			const std::optional<spk::Vector2UInt> &p_iconSize)
		{
			if (p_iconSize.has_value() == true)
			{
				return *p_iconSize;
			}

			if (p_icon.texture() == nullptr)
			{
				return {0, 0};
			}

			const spk::Vector2UInt textureSize = p_icon.texture()->size();
			const spk::Texture::Section &section = p_icon.section();

			return {
				static_cast<unsigned int>(static_cast<float>(textureSize.x) * section.size.x),
				static_cast<unsigned int>(static_cast<float>(textureSize.y) * section.size.y)};
		}

		[[nodiscard]] spk::Vector2UInt effectiveIconPadding(
			const spk::Vector2Int &p_cornerSize,
			const std::optional<spk::Vector2UInt> &p_iconPadding)
		{
			if (p_iconPadding.has_value() == true)
			{
				return *p_iconPadding;
			}

			return {
				static_cast<unsigned int>(p_cornerSize.x),
				static_cast<unsigned int>(p_cornerSize.y)};
		}
	}

	PushButton::PushButton(const std::string &p_name, spk::Widget *p_parent) :
		PushButton(p_name, "", p_parent)
	{
	}

	PushButton::PushButton(
		const std::string &p_name,
		std::string_view p_text,
		spk::Widget *p_parent) :
		PushButton(
			p_name,
			p_text,
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed),
			p_parent)
	{
	}

	PushButton::PushButton(
		const std::string &p_name,
		std::string_view p_text,
		const spk::WidgetStyle &p_style,
		spk::Widget *p_parent) :
		PushButton(p_name, p_text, p_style, p_style, p_parent)
	{
	}

	PushButton::PushButton(
		const std::string &p_name,
		std::string_view p_text,
		const spk::WidgetStyle &p_releasedStyle,
		const spk::WidgetStyle &p_pressedStyle,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent),
		_releasedBackground(p_name + "::releasedBackground", p_releasedStyle, this),
		_pressedBackground(p_name + "::pressedBackground", p_pressedStyle, this),
		_releasedLabel(p_name + "::releasedLabel", p_text, p_releasedStyle, this),
		_pressedLabel(p_name + "::pressedLabel", p_text, p_pressedStyle, this),
		_releasedIcon(p_name + "::releasedIcon", this),
		_pressedIcon(p_name + "::pressedIcon", this)
	{
		_pressedBackground.deactivate();
		_pressedLabel.deactivate();
		_releasedIcon.deactivate();
		_pressedIcon.deactivate();

		configureMinimalSizeGenerator([this]() {
			const spk::Vector2UInt releasedSize = _releasedLabel.minimalSize();
			const spk::Vector2UInt pressedSize = _pressedLabel.minimalSize();
			const spk::Vector2Int cornerSize = _releasedBackground.cornerSize();
			const spk::Vector2UInt minimumFrameSize = {
				static_cast<unsigned int>(cornerSize.x * 2),
				static_cast<unsigned int>(cornerSize.y * 2)};

			spk::Vector2UInt iconSize = {0, 0};
			if (_hasIcon == true)
			{
				const spk::Vector2UInt releasedIconSize = iconMinimalSize(_releasedIcon, _iconSize);
				const spk::Vector2UInt pressedIconSize = iconMinimalSize(_pressedIcon, _iconSize);
				const spk::Vector2UInt padding = effectiveIconPadding(cornerSize, _iconPadding);
				iconSize = {
					std::max(releasedIconSize.x, pressedIconSize.x) + padding.x * 2,
					std::max(releasedIconSize.y, pressedIconSize.y) + padding.y * 2};
			}

			return spk::Vector2UInt(
				std::max({releasedSize.x, pressedSize.x, iconSize.x, minimumFrameSize.x}),
				std::max({releasedSize.y, pressedSize.y, iconSize.y, minimumFrameSize.y}));
		});

		activate();
	}

	void PushButton::applyStyle(const spk::WidgetStyle &p_style)
	{
		applyStyle(p_style, p_style);
	}

	void PushButton::applyStyle(const spk::WidgetStyle &p_releasedStyle, const spk::WidgetStyle &p_pressedStyle)
	{
		_releasedBackground.applyStyle(p_releasedStyle);
		_pressedBackground.applyStyle(p_pressedStyle);
		_releasedLabel.applyStyle(p_releasedStyle);
		_pressedLabel.applyStyle(p_pressedStyle);
	}

	void PushButton::_refreshState()
	{
		if (_isPressed)
		{
			_releasedBackground.deactivate();
			_releasedLabel.deactivate();
			_releasedIcon.deactivate();
			if (_isFlat == false)
			{
				_pressedBackground.activate();
			}
			_pressedLabel.activate();
			if (_hasIcon == true)
			{
				_pressedIcon.activate();
			}
		}
		else
		{
			if (_isFlat == false)
			{
				_releasedBackground.activate();
			}
			_releasedLabel.activate();
			if (_hasIcon == true)
			{
				_releasedIcon.activate();
			}
			_pressedBackground.deactivate();
			_pressedLabel.deactivate();
			_pressedIcon.deactivate();
		}
	}

	void PushButton::_refreshIconGeometry()
	{
		const spk::Vector2Int cornerSize = _releasedBackground.cornerSize();
		const spk::Rect2D childRect(0, 0, geometry().width(), geometry().height());
		const spk::Vector2UInt padding = effectiveIconPadding(cornerSize, _iconPadding);
		spk::Rect2D iconRect = childRect.shrink(spk::Vector2Int(padding));

		if (_iconSize.has_value() == true)
		{
			const spk::Vector2UInt requestedSize = *_iconSize;
			const spk::Vector2UInt clampedSize = {
				std::min(requestedSize.x, iconRect.width()),
				std::min(requestedSize.y, iconRect.height())};
			iconRect = spk::Rect2D(
				{iconRect.x() + static_cast<int>((iconRect.width() - clampedSize.x) / 2),
				 iconRect.y() + static_cast<int>((iconRect.height() - clampedSize.y) / 2)},
				clampedSize);
		}

		_releasedIcon.setGeometry(iconRect);
		_pressedIcon.setGeometry(iconRect);
	}

	void PushButton::_onGeometryChange()
	{
		const spk::Rect2D childRect(0, 0, geometry().width(), geometry().height());
		_releasedBackground.setGeometry(childRect);
		_pressedBackground.setGeometry(childRect);
		_releasedLabel.setGeometry(childRect);
		_pressedLabel.setGeometry(childRect);
		_refreshIconGeometry();
	}

	PushButton::Contract PushButton::subscribeToClick(PushButton::Callback p_callback)
	{
		return _clickProvider.subscribe(std::move(p_callback));
	}

	void PushButton::useDefaultStyles()
	{
		applyStyle(
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed));
	}

	void PushButton::setText(const spk::Font::Text &p_text)
	{
		_releasedLabel.setText(p_text);
		_pressedLabel.setText(p_text);
	}

	void PushButton::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void PushButton::setAlignment(spk::HorizontalAlignment p_horizontal, spk::VerticalAlignment p_vertical)
	{
		_releasedLabel.setAlignment(p_horizontal, p_vertical);
		_pressedLabel.setAlignment(p_horizontal, p_vertical);
	}

	void PushButton::setIcon(std::shared_ptr<spk::Texture> p_texture, const spk::Texture::Section &p_section)
	{
		_releasedIcon.setTexture(p_texture);
		_releasedIcon.setSection(p_section);
		_pressedIcon.setTexture(std::move(p_texture));
		_pressedIcon.setSection(p_section);

		_hasIcon = true;
		_refreshIconGeometry();
		_refreshState();
	}

	void PushButton::setIcon(std::shared_ptr<spk::SpriteSheet> p_spriteSheet, size_t p_spriteID)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton icon sprite sheet cannot be null");
		}

		const spk::SpriteSheet::Sprite &sprite = p_spriteSheet->sprite(p_spriteID);
		setIcon(std::move(p_spriteSheet), sprite);
	}

	void PushButton::setIconSize(const spk::Vector2UInt &p_iconSize)
	{
		if (_iconSize == p_iconSize)
		{
			return;
		}

		_iconSize = p_iconSize;
		releaseMinimalSize();
		_refreshIconGeometry();
	}

	void PushButton::resetIconSize()
	{
		if (_iconSize.has_value() == false)
		{
			return;
		}

		_iconSize.reset();
		releaseMinimalSize();
		_refreshIconGeometry();
	}

	void PushButton::setIconPadding(const spk::Vector2UInt &p_iconPadding)
	{
		if (_iconPadding.has_value() == true && *_iconPadding == p_iconPadding)
		{
			return;
		}

		_iconPadding = p_iconPadding;
		releaseMinimalSize();
		_refreshIconGeometry();
	}

	void PushButton::resetIconPadding()
	{
		if (_iconPadding.has_value() == false)
		{
			return;
		}

		_iconPadding.reset();
		releaseMinimalSize();
		_refreshIconGeometry();
	}

	void PushButton::removeIcon()
	{
		_hasIcon = false;
		_releasedIcon.deactivate();
		_pressedIcon.deactivate();
	}

	void PushButton::setFlat(bool p_state)
	{
		if (_isFlat == p_state)
		{
			return;
		}

		_isFlat = p_state;
		if (_isFlat == true)
		{
			_releasedBackground.deactivate();
			_pressedBackground.deactivate();
		}
		_refreshState();
	}

	bool PushButton::hasIcon() const
	{
		return _hasIcon;
	}

	const std::optional<spk::Vector2UInt> &PushButton::iconSize() const
	{
		return _iconSize;
	}

	const std::optional<spk::Vector2UInt> &PushButton::iconPadding() const
	{
		return _iconPadding;
	}

	bool PushButton::isFlat() const
	{
		return _isFlat;
	}

	bool PushButton::isHovered() const
	{
		return _isHovered;
	}

	bool PushButton::isPressed() const
	{
		return _isPressed;
	}

	spk::Panel &PushButton::releasedBackground()
	{
		return _releasedBackground;
	}

	const spk::Panel &PushButton::releasedBackground() const
	{
		return _releasedBackground;
	}

	spk::Panel &PushButton::pressedBackground()
	{
		return _pressedBackground;
	}

	const spk::Panel &PushButton::pressedBackground() const
	{
		return _pressedBackground;
	}

	spk::TextLabel &PushButton::releasedLabel()
	{
		return _releasedLabel;
	}

	const spk::TextLabel &PushButton::releasedLabel() const
	{
		return _releasedLabel;
	}

	spk::TextLabel &PushButton::pressedLabel()
	{
		return _pressedLabel;
	}

	const spk::TextLabel &PushButton::pressedLabel() const
	{
		return _pressedLabel;
	}

	spk::ImageLabel &PushButton::releasedIcon()
	{
		return _releasedIcon;
	}

	const spk::ImageLabel &PushButton::releasedIcon() const
	{
		return _releasedIcon;
	}

	spk::ImageLabel &PushButton::pressedIcon()
	{
		return _pressedIcon;
	}

	const spk::ImageLabel &PushButton::pressedIcon() const
	{
		return _pressedIcon;
	}

	void PushButton::_onMouseMovedEvent(spk::MouseMovedEvent &p_event)
	{
		const bool hovered = absoluteGeometry().contains(p_event.device().position);
		if (_isHovered != hovered)
		{
			_isHovered = hovered;
		}
	}

	void PushButton::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || absoluteGeometry().contains(p_event.device().position) == false)
		{
			return;
		}

		_isHovered = true;
		_isPressed = true;
		takeFocus(FocusType::Mouse);
		_refreshState();
		p_event.consume();
	}

	void PushButton::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || _isPressed == false)
		{
			return;
		}

		_isPressed = false;
		_isHovered = absoluteGeometry().contains(p_event.device().position);
		releaseFocus(FocusType::Mouse);
		_refreshState();

		if (_isHovered == true)
		{
			_clickProvider.trigger();
			p_event.consume();
		}
	}
}
