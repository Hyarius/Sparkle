#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class TextLabel : public spk::Widget
	{
	private:
		std::shared_ptr<spk::Font> _font;
		spk::Font::Text _text;
		spk::Font::Size _textSize;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		float _depth = 0.0f;
		spk::HorizontalAlignment _horizontalAlignment = spk::HorizontalAlignment::Centered;
		spk::VerticalAlignment _verticalAlignment = spk::VerticalAlignment::Centered;
		spk::Vector2Int _padding = {0, 0};
		spk::WidgetStyle::Contract _styleEditionContract;

		void _bindStyle(const spk::WidgetStyle &p_style);
		void _configureSizeCache();

	protected:
		[[nodiscard]] spk::Vector2Int _textAnchor() const;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		explicit TextLabel(const std::string &p_name, spk::Widget *p_parent = nullptr);
		TextLabel(
			const std::string &p_name,
			std::string_view p_text,
			spk::Widget *p_parent = nullptr);
		TextLabel(
			const std::string &p_name,
			std::string_view p_text,
			const spk::WidgetStyle &p_style,
			spk::Widget *p_parent = nullptr);

		void applyStyle(const spk::WidgetStyle &p_style) override;
		void useDefaultStyle();
		void useStyle(const spk::WidgetStyle &p_style);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setText(const spk::Font::Text &p_text);
		void setText(std::string_view p_text);
		void setTextSize(const spk::Font::Size &p_textSize);
		void setGlyphColor(const spk::Color &p_color);
		void setOutlineColor(const spk::Color &p_color);
		void setDepth(float p_depth);
		void setHorizontalAlignment(spk::HorizontalAlignment p_alignment);
		void setVerticalAlignment(spk::VerticalAlignment p_alignment);
		void setAlignment(spk::HorizontalAlignment p_horizontalAlignment, spk::VerticalAlignment p_verticalAlignment);
		void setPadding(const spk::Vector2Int &p_padding);

		[[nodiscard]] const std::shared_ptr<spk::Font> &font() const;
		[[nodiscard]] const spk::Font::Text &text() const;
		[[nodiscard]] const spk::Font::Size &textSize() const;
		[[nodiscard]] const spk::Color &glyphColor() const;
		[[nodiscard]] const spk::Color &outlineColor() const;
		[[nodiscard]] float depth() const;
		[[nodiscard]] spk::HorizontalAlignment horizontalAlignment() const;
		[[nodiscard]] spk::VerticalAlignment verticalAlignment() const;
		[[nodiscard]] const spk::Vector2Int &padding() const;
	};
}
