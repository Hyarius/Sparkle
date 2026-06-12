#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "structures/math/spk_vector2.hpp"
#include "structures/graphics/rendering/command/spk_draw_font_render_command.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "structures/graphics/rendering/command/spk_render_command.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class TextRenderCommand : public spk::RenderCommand
	{
	private:
		spk::Font& _font;
		spk::Font::Atlas& _atlas;
		spk::Font::Text _text;
		spk::Font::Size _size;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		float _depth;
		spk::Vector2Int _anchor;
		spk::HorizontalAlignment _horizontalAlignment;
		spk::VerticalAlignment _verticalAlignment;
		bool _fontCommandOutdated = true;
		std::unique_ptr<spk::DrawFontRenderCommand> _fontCommand;
		spk::Font::Atlas::Contract _onAtlasEditionContract;

		void _rebuildFontCommand();

	public:
		TextRenderCommand(
			spk::Font& p_font,
			spk::Font::Text p_text,
			spk::Font::Size p_size,
			spk::Color p_glyphColor,
			spk::Color p_outlineColor,
			float p_depth,
			spk::Vector2Int p_anchor,
			spk::HorizontalAlignment p_horizontalAlignment = spk::HorizontalAlignment::Left,
			spk::VerticalAlignment p_verticalAlignment = spk::VerticalAlignment::Top);
		TextRenderCommand(
			spk::Font& p_font,
			std::string_view p_text,
			spk::Font::Size p_size,
			spk::Color p_glyphColor,
			spk::Color p_outlineColor,
			float p_depth,
			spk::Vector2Int p_anchor,
			spk::HorizontalAlignment p_horizontalAlignment = spk::HorizontalAlignment::Left,
			spk::VerticalAlignment p_verticalAlignment = spk::VerticalAlignment::Top);

		TextRenderCommand(const TextRenderCommand&) = delete;
		TextRenderCommand& operator=(const TextRenderCommand&) = delete;
		TextRenderCommand(TextRenderCommand&&) = delete;
		TextRenderCommand& operator=(TextRenderCommand&&) = delete;

		void execute(spk::RenderContext& p_renderContext) override;
	};
}
