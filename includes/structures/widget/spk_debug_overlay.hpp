#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class DebugOverlay : public spk::Widget
	{
	private:
		struct Row
		{
			std::unique_ptr<spk::HorizontalLayout> layout = std::make_unique<spk::HorizontalLayout>();
			std::vector<std::unique_ptr<spk::TextLabel>> labels;
		};

		spk::VerticalLayout _layout;
		std::vector<Row> _rows;

		size_t _outlineSize = 0;
		size_t _maxGlyphSize = 0;
		std::shared_ptr<spk::Font> _font;
		spk::Color _glyphColor = spk::Color(1.0f, 1.0f, 1.0f, 1.0f);
		spk::Color _outlineColor = spk::Color(0.0f, 0.0f, 0.0f, 1.0f);

		[[nodiscard]] std::unique_ptr<spk::TextLabel> _makeLabel(size_t p_row, size_t p_column);
		void _compose();
		void _applyFontsToFit();
		void _ensureRow(size_t p_row);
		void _ensureColumn(size_t p_row, size_t p_column);

	protected:
		void _onGeometryChange() override;

	public:
		explicit DebugOverlay(const std::string& p_name, spk::Widget* p_parent = nullptr);

		void configureRows(size_t p_rows, size_t p_defaultColumns);
		void setRowColumns(size_t p_row, size_t p_columns);

		void setText(size_t p_row, size_t p_column, std::string_view p_text);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setFontColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor);
		void setFontOutlineSize(size_t p_outlineSize);
		void setMaxGlyphSize(size_t p_maxGlyphSize);

		[[nodiscard]] size_t nbRow() const;
		[[nodiscard]] size_t nbColumn(size_t p_row) const;
		[[nodiscard]] const spk::TextLabel* label(size_t p_row, size_t p_column) const;
		[[nodiscard]] uint32_t labelHeight() const;
	};
}
