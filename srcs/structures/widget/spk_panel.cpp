#include "structures/widget/spk_panel.hpp"

#include <algorithm>
#include <stdexcept>

#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"

namespace
{
	[[nodiscard]] spk::Vector2Int defaultCornerSize(const spk::SpriteSheet &p_spriteSheet)
	{
		return {
			static_cast<int>(p_spriteSheet.size().x / p_spriteSheet.nbSprite().x),
			static_cast<int>(p_spriteSheet.size().y / p_spriteSheet.nbSprite().y)};
	}

	[[nodiscard]] spk::Vector2Int clampedCornerSize(
		const spk::Vector2Int &p_cornerSize,
		const spk::Rect2D &p_geometry)
	{
		return {
			std::min(p_cornerSize.x, static_cast<int>(p_geometry.width() / 2)),
			std::min(p_cornerSize.y, static_cast<int>(p_geometry.height() / 2))};
	}
}

namespace spk
{
	Panel::Panel(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		useDefaultStyle();
		activate();
	}

	Panel::Panel(
		const std::string &p_name,
		const spk::WidgetStyle &p_style,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		useStyle(p_style);
		activate();
	}

	Panel::Panel(
		const std::string &p_name,
		std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
		spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setSpriteSheet(std::move(p_spriteSheet));
		activate();
	}

	void Panel::_bindStyle(const spk::WidgetStyle &p_style)
	{
		_styleEditionContract = p_style.subscribeToEdition([this](const spk::WidgetStyle &p_editedStyle) {
			applyStyle(p_editedStyle);
		});
	}

	void Panel::applyStyle(const spk::WidgetStyle &p_style)
	{
		setSpriteSheet(p_style.nineSliceSpriteSheet());
		setCornerSize(p_style.nineSliceCornerSize());
	}

	void Panel::useDefaultStyle()
	{
		_bindStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
		applyStyle(spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default));
	}

	void Panel::useStyle(const spk::WidgetStyle &p_style)
	{
		_bindStyle(p_style);
		applyStyle(p_style);
	}

	spk::RenderUnit Panel::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		if (_spriteSheet != nullptr && geometry().empty() == false)
		{
			builder.emplace<spk::NineSliceRenderCommand>(
				*_spriteSheet,
				geometry(),
				clampedCornerSize(_cornerSize, geometry()),
				_depth);
		}

		return builder.build();
	}

	void Panel::setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet)
	{
		if (p_spriteSheet == nullptr)
		{
			throw std::invalid_argument("Panel sprite sheet cannot be null");
		}

		if (p_spriteSheet->nbSprite() != spk::Vector2UInt{3, 3})
		{
			throw std::invalid_argument("Panel sprite sheet must contain a 3x3 grid");
		}

		_spriteSheet = std::move(p_spriteSheet);
		_cornerSize = defaultCornerSize(*_spriteSheet);
		invalidateRenderUnit();
	}

	void Panel::setCornerSize(const spk::Vector2Int &p_cornerSize)
	{
		if (p_cornerSize.x < 0 || p_cornerSize.y < 0)
		{
			throw std::invalid_argument("Panel corner size cannot be negative");
		}

		if (_cornerSize == p_cornerSize)
		{
			return;
		}

		_cornerSize = p_cornerSize;
		invalidateRenderUnit();
	}

	void Panel::setDepth(float p_depth)
	{
		if (_depth == p_depth)
		{
			return;
		}

		_depth = p_depth;
		invalidateRenderUnit();
	}

	const std::shared_ptr<spk::SpriteSheet> &Panel::spriteSheet() const
	{
		return _spriteSheet;
	}

	const spk::Vector2Int &Panel::cornerSize() const
	{
		return _cornerSize;
	}

	float Panel::depth() const
	{
		return _depth;
	}
}
