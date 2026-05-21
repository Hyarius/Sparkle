#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <memory>
#include <string>

#include "math/spk_vector2.hpp"
#include "rendering/render_command/spk_draw_font_render_command.hpp"
#include "rendering/spk_color.hpp"
#include "rendering/spk_font.hpp"
#include "rendering/spk_horizontal_alignment.hpp"
#include "rendering/spk_render_command.hpp"
#include "rendering/spk_vertical_alignment.hpp"

namespace spk
{
	class TextRenderCommand : public spk::RenderCommand
	{
	private:
		const spk::Font& _font;
		std::wstring _text;
		spk::Font::Size _size;
		spk::Color _color;
		float _depth = 0.0f;
		spk::Vector2Int _anchor;
		spk::HorizontalAlignment _horizontalAlignment;
		spk::VerticalAlignment _verticalAlignment;

		mutable std::unique_ptr<spk::DrawFontRenderCommand> _fontCommand;

		void _ensureFontCommand() const;
		[[nodiscard]] spk::Vector2Int _computeBaselinePosition() const;

	public:
		TextRenderCommand(
			const spk::Font& p_font,
			std::wstring p_text,
			spk::Font::Size p_size,
			spk::Color p_color,
			float p_depth,
			spk::Vector2Int p_anchor,
			spk::HorizontalAlignment p_horizontalAlignment = spk::HorizontalAlignment::Left,
			spk::VerticalAlignment p_verticalAlignment = spk::VerticalAlignment::Top);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
