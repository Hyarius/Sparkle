#include "structures/widget/spk_push_button.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"
#include "structures/graphics/rendering/command/spk_text_render_command.hpp"

namespace
{
	[[nodiscard]] bool sameColor(const spk::Color& p_left, const spk::Color& p_right)
	{
		return p_left.r == p_right.r &&
			p_left.g == p_right.g &&
			p_left.b == p_right.b &&
			p_left.a == p_right.a;
	}

	[[nodiscard]] spk::Vector2Int centeredAnchor(const spk::Rect2D& p_rect)
	{
		return {
			p_rect.left() + static_cast<int>(p_rect.width() / 2),
			p_rect.top() + static_cast<int>(p_rect.height() / 2)
		};
	}

	[[nodiscard]] spk::Vector2Int clampedCornerSize(
		const spk::Vector2Int& p_cornerSize,
		const spk::Rect2D& p_geometry)
	{
		return {
			std::min(p_cornerSize.x, static_cast<int>(p_geometry.width() / 2)),
			std::min(p_cornerSize.y, static_cast<int>(p_geometry.height() / 2))
		};
	}
}

namespace spk
{
	PushButton::PushButton(const std::string& p_name, spk::Widget* p_parent) :
		PushButton(p_name, "", p_parent)
	{
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		spk::Widget* p_parent) :
		spk::Panel(
			p_name,
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).nineSliceSpriteSheet(),
			p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		useDefaultStyles();
		_refreshVisualStyle();
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		const spk::WidgetStyle& p_style,
		spk::Widget* p_parent) :
		PushButton(p_name, p_text, p_style, p_style, p_parent)
	{
	}

	PushButton::PushButton(
		const std::string& p_name,
		std::string_view p_text,
		const spk::WidgetStyle& p_releasedStyle,
		const spk::WidgetStyle& p_pressedStyle,
		spk::Widget* p_parent) :
		spk::Panel(p_name, p_releasedStyle.nineSliceSpriteSheet(), p_parent),
		_text(spk::Font::textFromUTF8(p_text))
	{
		useStyles(p_releasedStyle, p_pressedStyle);
		_refreshVisualStyle();
	}

	void PushButton::_applyStyle(const spk::WidgetStyle& p_style)
	{
		_releasedStyle = _visualStyleFrom(p_style);
		_pressedStyle = _releasedStyle;
		_refreshVisualStyle();
	}

	PushButton::VisualStyle PushButton::_visualStyleFrom(const spk::WidgetStyle& p_style)
	{
		return {
			.spriteSheet = p_style.nineSliceSpriteSheet(),
			.cornerSize = p_style.nineSliceCornerSize(),
			.font = p_style.font(),
			.textSize = p_style.textSize(),
			.glyphColor = p_style.glyphColor(),
			.outlineColor = p_style.outlineColor(),
			.textPadding = p_style.textPadding()
		};
	}

	void PushButton::_bindReleasedStyle(const spk::WidgetStyle& p_style)
	{
		_releasedStyle = _visualStyleFrom(p_style);
		_releasedStyleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle& p_editedStyle)
		{
			_releasedStyle = _visualStyleFrom(p_editedStyle);
			if (_isPressed == false)
			{
				_applyVisualStyle(_releasedStyle);
			}
		});
	}

	void PushButton::_bindPressedStyle(const spk::WidgetStyle& p_style)
	{
		_pressedStyle = _visualStyleFrom(p_style);
		_pressedStyleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle& p_editedStyle)
		{
			_pressedStyle = _visualStyleFrom(p_editedStyle);
			if (_isPressed == true)
			{
				_applyVisualStyle(_pressedStyle);
			}
		});
	}

	PushButton::Contract PushButton::subscribeToClick(Callback p_callback)
	{
		return _clickProvider.subscribe(std::move(p_callback));
	}

	void PushButton::useDefaultStyles()
	{
		useStyles(
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default),
			spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::DefaultPressed));
	}

	void PushButton::useStyles(const spk::WidgetStyle& p_releasedStyle, const spk::WidgetStyle& p_pressedStyle)
	{
		_bindReleasedStyle(p_releasedStyle);
		_bindPressedStyle(p_pressedStyle);
		_refreshVisualStyle();
	}

	bool PushButton::_containsMouse(const spk::Mouse& p_mouse) const
	{
		return absoluteGeometry().contains(p_mouse.position);
	}

	void PushButton::_applyVisualStyle(const VisualStyle& p_style)
	{
		setSpriteSheet(p_style.spriteSheet);
		setCornerSize(p_style.cornerSize);
		setFont(p_style.font);
		setTextSize(p_style.textSize);
		setGlyphColor(p_style.glyphColor);
		setOutlineColor(p_style.outlineColor);
		setTextPadding(p_style.textPadding);
	}

	void PushButton::_refreshVisualStyle()
	{
		_applyVisualStyle(_isPressed ? _pressedStyle : _releasedStyle);
	}

	spk::RenderUnit PushButton::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (spriteSheet() != nullptr && absoluteGeometry().empty() == false)
		{
			builder.emplace<spk::NineSliceRenderCommand>(
				*spriteSheet(),
				absoluteGeometry(),
				clampedCornerSize(cornerSize(), absoluteGeometry()),
				depth());
		}

		if (_font != nullptr && _text.empty() == false && absoluteGeometry().empty() == false)
		{
			builder.emplace<spk::TextRenderCommand>(
				*_font,
				_text,
				_textSize,
				_glyphColor,
				_outlineColor,
				_textDepth,
				centeredAnchor(absoluteGeometry().shrink(_textPadding)),
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered);
		}

		return builder.build();
	}

	void PushButton::_onMouseMovedEvent(spk::MouseMovedEvent& p_event)
	{
		const bool hovered = _containsMouse(p_event.device());
		if (_isHovered != hovered)
		{
			_isHovered = hovered;
		}
	}

	void PushButton::_onMouseLeftEvent(spk::MouseLeftWindowEvent& p_event)
	{
		(void)p_event;

		if (_isHovered == false && _isPressed == false)
		{
			return;
		}

		_isHovered = false;
		_isPressed = false;
		_refreshVisualStyle();
	}

	void PushButton::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left || _containsMouse(p_event.device()) == false)
		{
			return;
		}

		_isHovered = true;
		_isPressed = true;
		takeFocus(FocusType::Mouse);
		_refreshVisualStyle();
		p_event.consume();
	}

	void PushButton::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event)
	{
		if (p_event->button != spk::Mouse::Left || _isPressed == false)
		{
			return;
		}

		_isPressed = false;
		_isHovered = _containsMouse(p_event.device());
		_refreshVisualStyle();

		if (_isHovered == true)
		{
			_clickProvider.trigger();
			p_event.consume();
		}
	}

	void PushButton::setReleasedSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton released sprite sheet cannot be null");
		}

		_releasedStyle.spriteSheet = std::move(p_spriteSheet);
		if (_isPressed == false)
		{
			_refreshVisualStyle();
		}
	}

	void PushButton::setPressedSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("PushButton pressed sprite sheet cannot be null");
		}

		_pressedStyle.spriteSheet = std::move(p_spriteSheet);
		if (_isPressed == true)
		{
			_refreshVisualStyle();
		}
	}

	void PushButton::setFont(std::shared_ptr<spk::Font> p_font)
	{
		if (p_font == nullptr)
		{
			throw std::invalid_argument("PushButton font cannot be null");
		}

		_font = std::move(p_font);
		invalidateRenderUnit();
	}

	void PushButton::setText(const spk::Font::Text& p_text)
	{
		if (_text == p_text)
		{
			return;
		}

		_text = p_text;
		invalidateRenderUnit();
	}

	void PushButton::setText(std::string_view p_text)
	{
		setText(spk::Font::textFromUTF8(p_text));
	}

	void PushButton::setTextSize(const spk::Font::Size& p_textSize)
	{
		if (_textSize == p_textSize)
		{
			return;
		}

		_textSize = p_textSize;
		invalidateRenderUnit();
	}

	void PushButton::setGlyphColor(const spk::Color& p_color)
	{
		if (sameColor(_glyphColor, p_color) == true)
		{
			return;
		}

		_glyphColor = p_color;
		invalidateRenderUnit();
	}

	void PushButton::setOutlineColor(const spk::Color& p_color)
	{
		if (sameColor(_outlineColor, p_color) == true)
		{
			return;
		}

		_outlineColor = p_color;
		invalidateRenderUnit();
	}

	void PushButton::setTextPadding(const spk::Vector2Int& p_padding)
	{
		if (_textPadding == p_padding)
		{
			return;
		}

		_textPadding = p_padding;
		invalidateRenderUnit();
	}

	void PushButton::setTextDepth(float p_depth)
	{
		if (_textDepth == p_depth)
		{
			return;
		}

		_textDepth = p_depth;
		invalidateRenderUnit();
	}

	bool PushButton::isHovered() const
	{
		return _isHovered;
	}

	bool PushButton::isPressed() const
	{
		return _isPressed;
	}

	const spk::Font::Text& PushButton::text() const
	{
		return _text;
	}

	const spk::Font::Size& PushButton::textSize() const
	{
		return _textSize;
	}

	const spk::Color& PushButton::glyphColor() const
	{
		return _glyphColor;
	}

	const spk::Color& PushButton::outlineColor() const
	{
		return _outlineColor;
	}

	const spk::Vector2Int& PushButton::textPadding() const
	{
		return _textPadding;
	}
}
