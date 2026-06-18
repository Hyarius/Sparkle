#include "structures/widget/spk_debug_overlay.hpp"

#include <stdexcept>
#include <utility>

#include "structures/widget/spk_widget_style.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::Font> defaultFont()
	{
		return spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).font();
	}
}

namespace spk
{
	DebugOverlay::DebugOverlay(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_layout.setElementPadding({4, 4});
		activate();
	}

	std::unique_ptr<spk::TextLabel> DebugOverlay::_makeLabel(size_t p_row, size_t p_column)
	{
		auto newLabel = std::make_unique<spk::TextLabel>(name() + "::label[" + std::to_string(p_column) + "][" + std::to_string(p_row) + "]", this);
		newLabel->setFont((_font != nullptr) ? _font : defaultFont());
		newLabel->setGlyphColor(_glyphColor);
		newLabel->setOutlineColor(_outlineColor);
		newLabel->setAlignment(spk::HorizontalAlignment::Left, spk::VerticalAlignment::Centered);
		newLabel->setPadding({0, 0});
		return newLabel;
	}

	void DebugOverlay::_compose()
	{
		_layout.clear();

		for (Row& row : _rows)
		{
			row.layout->clear();

			for (auto& label : row.labels)
			{
				if (label != nullptr)
				{
					row.layout->addWidget(label.get(), spk::Layout::SizePolicy::Extend);
				}
			}

			_layout.addLayout(row.layout.get(), spk::Layout::SizePolicy::Extend);
		}
	}

	void DebugOverlay::_applyFontsToFit()
	{
		for (Row& row : _rows)
		{
			for (auto& label : row.labels)
			{
				if (label == nullptr || label->font() == nullptr || label->text().empty() == true)
				{
					continue;
				}

				const spk::Vector2UInt area = label->geometry().size;
				if (area.x == 0 || area.y == 0)
				{
					continue;
				}

				const spk::Font::Size optimalSize = label->font()->computeOptimalTextSize(label->text(), 0.0f, area);

				size_t glyphSize = optimalSize.glyph;
				if (glyphSize > _outlineSize * 2)
				{
					glyphSize -= _outlineSize * 2;
				}
				if (_maxGlyphSize > 0 && glyphSize > _maxGlyphSize)
				{
					glyphSize = _maxGlyphSize;
				}

				label->setTextSize(spk::Font::Size(glyphSize, _outlineSize));
			}
		}
	}

	void DebugOverlay::_ensureRow(size_t p_row)
	{
		if (p_row < _rows.size())
		{
			return;
		}

		_rows.resize(p_row + 1);
	}

	void DebugOverlay::_ensureColumn(size_t p_row, size_t p_column)
	{
		_ensureRow(p_row);
		Row& row = _rows[p_row];
		if (p_column < row.labels.size())
		{
			return;
		}

		const size_t oldSize = row.labels.size();
		row.labels.resize(p_column + 1);
		for (size_t c = oldSize; c <= p_column; ++c)
		{
			row.labels[c] = _makeLabel(p_row, c);
			row.labels[c]->activate();
		}
	}

	void DebugOverlay::_onGeometryChange()
	{
		_compose();
		_layout.setGeometry(geometry().atOrigin());
		_applyFontsToFit();
	}

	void DebugOverlay::configureRows(size_t p_rows, size_t p_defaultColumns)
	{
		_rows.clear();
		_rows.resize(p_rows);
		for (size_t r = 0; r < p_rows; ++r)
		{
			setRowColumns(r, p_defaultColumns);
		}
		_onGeometryChange();
	}

	void DebugOverlay::setRowColumns(size_t p_row, size_t p_columns)
	{
		_ensureRow(p_row);
		Row& row = _rows[p_row];

		const size_t current = row.labels.size();
		if (p_columns == current)
		{
			return;
		}

		row.labels.resize(p_columns);
		for (size_t c = current; c < p_columns; ++c)
		{
			row.labels[c] = _makeLabel(p_row, c);
			row.labels[c]->activate();
		}

		_onGeometryChange();
	}

	void DebugOverlay::setText(size_t p_row, size_t p_column, std::string_view p_text)
	{
		_ensureColumn(p_row, p_column);
		_rows[p_row].labels[p_column]->setText(p_text);
		_applyFontsToFit();
	}

	void DebugOverlay::setFont(std::shared_ptr<spk::Font> p_font)
	{
		_font = std::move(p_font);

		for (Row& row : _rows)
		{
			for (auto& label : row.labels)
			{
				if (label != nullptr)
				{
					label->setFont((_font != nullptr) ? _font : defaultFont());
				}
			}
		}

		_onGeometryChange();
	}

	void DebugOverlay::setFontColor(const spk::Color& p_glyphColor, const spk::Color& p_outlineColor)
	{
		_glyphColor = p_glyphColor;
		_outlineColor = p_outlineColor;

		for (Row& row : _rows)
		{
			for (auto& label : row.labels)
			{
				if (label != nullptr)
				{
					label->setGlyphColor(_glyphColor);
					label->setOutlineColor(_outlineColor);
				}
			}
		}
	}

	void DebugOverlay::setFontOutlineSize(size_t p_outlineSize)
	{
		_outlineSize = p_outlineSize;
		_applyFontsToFit();
	}

	void DebugOverlay::setMaxGlyphSize(size_t p_maxGlyphSize)
	{
		_maxGlyphSize = p_maxGlyphSize;
		_applyFontsToFit();
	}

	size_t DebugOverlay::nbRow() const
	{
		return _rows.size();
	}

	size_t DebugOverlay::nbColumn(size_t p_row) const
	{
		if (p_row >= _rows.size())
		{
			return 0;
		}
		return _rows[p_row].labels.size();
	}

	const spk::TextLabel* DebugOverlay::label(size_t p_row, size_t p_column) const
	{
		if (p_row >= _rows.size() || p_column >= _rows[p_row].labels.size())
		{
			return nullptr;
		}
		return _rows[p_row].labels[p_column].get();
	}

	uint32_t DebugOverlay::labelHeight() const
	{
		if (_rows.empty() == true || _rows[0].labels.empty() == true || _rows[0].labels[0] == nullptr)
		{
			return 0u;
		}
		return _rows[0].labels[0]->geometry().size.y;
	}
}
