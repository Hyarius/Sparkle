#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class TextArea : public spk::Widget
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _spriteSheet;
		spk::Vector2Int _cornerSize;
		std::shared_ptr<spk::Font> _font;
		spk::Font::Size _textSize;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		float _depth = 0.0f;

		spk::Font::Text _text;
		size_t _linePadding = 0;
		bool _backgroundVisible = true;
		spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Left;
		spk::VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Top;

		spk::WidgetStyle::Contract _styleEditionContract;

		void _bindStyle(const spk::WidgetStyle &p_style);

		[[nodiscard]] spk::Rect2D _innerGeometry() const;
		[[nodiscard]] std::vector<spk::Font::Text> _composeLines(unsigned int p_availableWidth) const;
		[[nodiscard]] unsigned int _lineHeight() const;

	protected:
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		explicit TextArea(const std::string &p_name, spk::Widget *p_parent = nullptr);
		TextArea(
			const std::string &p_name,
			std::string_view p_text,
			spk::Widget *p_parent = nullptr);
		TextArea(
			const std::string &p_name,
			std::string_view p_text,
			const spk::WidgetStyle &p_style,
			spk::Widget *p_parent = nullptr);

		void applyStyle(const spk::WidgetStyle &p_style) override;
		void useDefaultStyle();
		void useStyle(const spk::WidgetStyle &p_style);

		void setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2Int &p_cornerSize);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setTextSize(const spk::Font::Size &p_textSize);
		void setGlyphColor(const spk::Color &p_color);
		void setOutlineColor(const spk::Color &p_color);
		void setDepth(float p_depth);

		void setText(const spk::Font::Text &p_text);
		void setText(std::string_view p_text);
		void setLinePadding(size_t p_linePadding);
		void setBackgroundVisible(bool p_state);
		[[nodiscard]] bool isBackgroundVisible() const;
		void setAlignment(spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment);

		[[nodiscard]] spk::Vector2UInt computeMinimalSize(unsigned int p_availableWidth) const;

		[[nodiscard]] const spk::Font::Text &text() const;
		[[nodiscard]] size_t linePadding() const;
		[[nodiscard]] spk::HorizontalAlignment horizontalAlignment() const;
		[[nodiscard]] spk::VerticalAlignment verticalAlignment() const;

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet> &spriteSheet() const;
		[[nodiscard]] const spk::Vector2Int &cornerSize() const;
		[[nodiscard]] const std::shared_ptr<spk::Font> &font() const;
		[[nodiscard]] const spk::Font::Size &textSize() const;
		[[nodiscard]] const spk::Color &glyphColor() const;
		[[nodiscard]] const spk::Color &outlineColor() const;
		[[nodiscard]] float depth() const;
	};
}
