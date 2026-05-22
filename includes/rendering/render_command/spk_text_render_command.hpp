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
		std::unique_ptr<spk::DrawFontRenderCommand> _fontCommand;

	public:
		TextRenderCommand(
			const spk::Font& p_font,
			std::wstring p_text,
			spk::Font::Size p_size,
			spk::Color p_glyphColor,
			spk::Color p_outlineColor,
			float p_depth,
			spk::Vector2Int p_anchor,
			spk::HorizontalAlignment p_horizontalAlignment = spk::HorizontalAlignment::Left,
			spk::VerticalAlignment p_verticalAlignment = spk::VerticalAlignment::Top);

		void execute(spk::IRenderContext& p_renderContext) override;
	};
}

#endif
