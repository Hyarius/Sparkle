#pragma once

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "structures/widget/spk_layout.hpp"

namespace spk
{
	class GridLayout : public spk::Layout
	{
	protected:
		using Layout::addWidget;
		using Layout::removeWidget;

		spk::Vector2UInt _size = {0, 0};

		[[nodiscard]] size_t _idx(size_t p_row, size_t p_column) const
		{
			return p_row * _size.x + p_column;
		}

		void _resizeGrid(size_t p_rows, size_t p_columns)
		{
			if (p_rows == _size.y && p_columns == _size.x)
			{
				return;
			}

			std::vector<std::unique_ptr<Element>> newElements(p_rows * p_columns);

			const size_t copyRows = std::min(static_cast<size_t>(_size.y), p_rows);
			const size_t copyColumns = std::min(static_cast<size_t>(_size.x), p_columns);

			for (size_t r = 0; r < copyRows; ++r)
			{
				for (size_t c = 0; c < copyColumns; ++c)
				{
					newElements[r * p_columns + c] = std::move(_elements[r * _size.x + c]);
				}
			}

			_elements.swap(newElements);
			_size.y = static_cast<unsigned int>(p_rows);
			_size.x = static_cast<unsigned int>(p_columns);
		}

		void _ensureSize(size_t p_rows, size_t p_columns)
		{
			if (p_rows <= _size.y && p_columns <= _size.x)
			{
				return;
			}
			_resizeGrid(
				std::max(static_cast<size_t>(_size.y), p_rows),
				std::max(static_cast<size_t>(_size.x), p_columns));
		}

	private:
		[[nodiscard]] spk::Vector2UInt _computeMinimalSize() const
		{
			return _computeMinimalSizeFor(std::numeric_limits<spk::Vector2UInt>::max());
		}

		[[nodiscard]] static spk::Vector2UInt _elementMinimalSizeFor(
			const Element &p_element,
			const spk::Vector2UInt &p_availableSize)
		{
			return p_element.minimalSizeFor(p_availableSize);
		}

		[[nodiscard]] static size_t _clampDimension(size_t p_value, size_t p_minimum, size_t p_maximum)
		{
			return std::clamp(p_value, p_minimum, std::max(p_minimum, p_maximum));
		}

		[[nodiscard]] static size_t _effectiveElementWidth(const Element &p_element, size_t p_cellWidth)
		{
			switch (p_element.sizePolicy())
			{
			case SizePolicy::Fixed:
				return p_element.fixedSize().x;

			case SizePolicy::Maximum:
				return p_element.maximalSize().x;

			default:
			{
				const spk::Vector2UInt maximalSize = p_element.maximalSize();
				const size_t queryWidth = std::min(p_cellWidth, static_cast<size_t>(maximalSize.x));
				const spk::Vector2UInt minimalSize = p_element.minimalSizeFor(
					{static_cast<unsigned int>(queryWidth), std::numeric_limits<uint32_t>::max()});
				return _clampDimension(
					p_cellWidth,
					static_cast<size_t>(minimalSize.x),
					static_cast<size_t>(maximalSize.x));
			}
			}
		}

		[[nodiscard]] static size_t _effectiveElementHeight(
			const Element &p_element,
			size_t p_cellHeight,
			size_t p_effectiveWidth)
		{
			switch (p_element.sizePolicy())
			{
			case SizePolicy::Fixed:
				return p_element.fixedSize().y;

			case SizePolicy::Maximum:
				return p_element.maximalSize().y;

			default:
			{
				const spk::Vector2UInt minimalSize = _elementMinimalSizeFor(
					p_element,
					{static_cast<unsigned int>(p_effectiveWidth),
					 static_cast<unsigned int>(p_cellHeight)});
				const spk::Vector2UInt maximalSize = p_element.maximalSize();
				return _clampDimension(
					p_cellHeight,
					static_cast<size_t>(minimalSize.y),
					static_cast<size_t>(maximalSize.y));
			}
			}
		}

		[[nodiscard]] spk::Vector2UInt _computeMinimalSizeFor(const spk::Vector2UInt &p_availableSize) const
		{
			if (_size.y == 0 || _size.x == 0)
			{
				return {0u, 0u};
			}

			std::vector<size_t> columnWidth(_size.x, 0);
			std::vector<size_t> rowHeight(_size.y, 0);

			const size_t totalPaddingX = (_size.x > 1) ? static_cast<size_t>(_size.x - 1) * _elementPadding.x : 0;
			const size_t totalPaddingY = (_size.y > 1) ? static_cast<size_t>(_size.y - 1) * _elementPadding.y : 0;
			const size_t availableContentWidth =
				(p_availableSize.x > totalPaddingX) ? static_cast<size_t>(p_availableSize.x) - totalPaddingX : 0;
			const size_t availableContentHeight =
				(p_availableSize.y > totalPaddingY) ? static_cast<size_t>(p_availableSize.y) - totalPaddingY : 0;

			for (size_t x = 0; x < _size.x; x++)
			{
				for (size_t y = 0; y < _size.y; y++)
				{
					const auto &element = _elements[_idx(y, x)];

					if (element == nullptr || (element->widget() == nullptr && element->layout() == nullptr))
					{
						continue;
					}

					const spk::Vector2UInt maximalSize = element->maximalSize();
					const size_t queryWidth = std::min(availableContentWidth, static_cast<size_t>(maximalSize.x));
					const size_t queryHeight = std::min(availableContentHeight, static_cast<size_t>(maximalSize.y));

					switch (element->sizePolicy())
					{
					case SizePolicy::Fixed:
					{
						const spk::Vector2UInt fixedSizeValue = element->fixedSize();
						columnWidth[x] = std::max(columnWidth[x], static_cast<size_t>(fixedSizeValue.x));
						break;
					}
					case SizePolicy::Maximum:
					{
						columnWidth[x] = std::max(columnWidth[x], static_cast<size_t>(maximalSize.x));
						break;
					}
					default:
					{
						const spk::Vector2UInt minimalValue = element->minimalSizeFor(
							{static_cast<unsigned int>(queryWidth), static_cast<unsigned int>(queryHeight)});
						columnWidth[x] = std::max(
							columnWidth[x],
							std::min(static_cast<size_t>(minimalValue.x), static_cast<size_t>(maximalSize.x)));
						break;
					}
					}
				}
			}

			for (size_t x = 0; x < _size.x; x++)
			{
				for (size_t y = 0; y < _size.y; y++)
				{
					const auto &element = _elements[_idx(y, x)];

					if (element == nullptr || (element->widget() == nullptr && element->layout() == nullptr))
					{
						continue;
					}

					const size_t effectiveWidth = _effectiveElementWidth(*element, columnWidth[x]);

					switch (element->sizePolicy())
					{
					case SizePolicy::Fixed:
					{
						const spk::Vector2UInt fixedSizeValue = element->fixedSize();
						rowHeight[y] = std::max(rowHeight[y], static_cast<size_t>(fixedSizeValue.y));
						break;
					}
					case SizePolicy::Maximum:
					{
						const spk::Vector2UInt maximalSizeValue = element->maximalSize();
						rowHeight[y] = std::max(rowHeight[y], static_cast<size_t>(maximalSizeValue.y));
						break;
					}
					default:
					{
						const spk::Vector2UInt minimalValue = _elementMinimalSizeFor(
							*element,
							{static_cast<unsigned int>(effectiveWidth),
							 static_cast<unsigned int>(availableContentHeight)});
						const spk::Vector2UInt maximalSizeValue = element->maximalSize();
						rowHeight[y] = std::max(
							rowHeight[y],
							std::min(static_cast<size_t>(minimalValue.y), static_cast<size_t>(maximalSizeValue.y)));
						break;
					}
					}
				}
			}

			const size_t totalWidth = std::accumulate(columnWidth.begin(), columnWidth.end(), size_t{0}) + totalPaddingX;
			const size_t totalHeight = std::accumulate(rowHeight.begin(), rowHeight.end(), size_t{0}) + totalPaddingY;

			return {
				static_cast<unsigned int>(std::min(totalWidth, static_cast<size_t>(std::numeric_limits<uint32_t>::max()))),
				static_cast<unsigned int>(std::min(totalHeight, static_cast<size_t>(std::numeric_limits<uint32_t>::max())))};
		}

	public:
		GridLayout()
		{
			configureMinimalSizeGenerator([this]() {
				return _computeMinimalSize();
			});
			configureFixedSizeGenerator([this]() {
				return _computeMinimalSize();
			});
		}

		[[nodiscard]] spk::Vector2UInt minimalSizeFor(const spk::Vector2UInt &p_availableSize) const override
		{
			return _computeMinimalSizeFor(p_availableSize);
		}

		void clear() override
		{
			_size = {0, 0};
			spk::Layout::clear();
		}

		[[nodiscard]] size_t rowCount() const
		{
			return _size.y;
		}

		[[nodiscard]] size_t columnCount() const
		{
			return _size.x;
		}

		void addEmptyRow()
		{
			_ensureSize(_size.y + 1, (_size.x == 0) ? 1 : _size.x);
		}

		void addEmptyColumn()
		{
			_ensureSize((_size.y == 0) ? 1 : _size.y, _size.x + 1);
		}

		Element *setWidget(size_t p_column, size_t p_row, spk::Widget *p_widget, SizePolicy p_sizePolicy = SizePolicy::Extend)
		{
			if (p_widget == nullptr)
			{
				throw std::invalid_argument("GridLayout cannot hold a null widget");
			}

			_ensureSize(p_row + 1, p_column + 1);
			const size_t index = _idx(p_row, p_column);

			if (_elements[index] == nullptr)
			{
				_elements[index] = std::make_unique<Element>(p_widget, p_sizePolicy, spk::Vector2UInt{1, 1});
			}
			else
			{
				*_elements[index] = Element(p_widget, p_sizePolicy, spk::Vector2UInt{1, 1});
			}
			return _elements[index].get();
		}

		void setGeometry(const spk::Rect2D &p_geometry) override
		{
			if (_size.x == 0 || _size.y == 0)
			{
				return;
			}

			const size_t totalPaddingX = (_size.x > 1) ? static_cast<size_t>(_size.x - 1) * _elementPadding.x : 0;
			const size_t totalPaddingY = (_size.y > 1) ? static_cast<size_t>(_size.y - 1) * _elementPadding.y : 0;
			const size_t availableContentWidth =
				(p_geometry.size.x > totalPaddingX) ? static_cast<size_t>(p_geometry.size.x) - totalPaddingX : 0;

			std::vector<bool> isExtendableOnX(_size.x, false);
			size_t nbExtendableOnX = 0;
			std::vector<bool> isExtendableOnY(_size.y, false);
			size_t nbExtendableOnY = 0;

			std::vector<size_t> minimalSizeOnX(_size.x, 0);
			std::vector<size_t> minimalSizeOnY(_size.y, 0);

			for (size_t x = 0; x < _size.x; x++)
			{
				for (size_t y = 0; y < _size.y; y++)
				{
					const auto &element = _elements[_idx(y, x)];

					if (element == nullptr || (element->widget() == nullptr && element->layout() == nullptr))
					{
						continue;
					}

					switch (element->sizePolicy())
					{
					case SizePolicy::Extend:
					{
						if (isExtendableOnX[x] == false)
						{
							isExtendableOnX[x] = true;
							nbExtendableOnX++;
						}
						if (isExtendableOnY[y] == false)
						{
							isExtendableOnY[y] = true;
							nbExtendableOnY++;
						}
						break;
					}
					case SizePolicy::HorizontalExtend:
					{
						if (isExtendableOnX[x] == false)
						{
							isExtendableOnX[x] = true;
							nbExtendableOnX++;
						}
						break;
					}
					case SizePolicy::VerticalExtend:
					{
						if (isExtendableOnY[y] == false)
						{
							isExtendableOnY[y] = true;
							nbExtendableOnY++;
						}
						break;
					}
					default:
					{
						break;
					}
					}

					switch (element->sizePolicy())
					{
					case SizePolicy::Fixed:
					{
						const spk::Vector2UInt fixedSizeValue = element->fixedSize();
						minimalSizeOnX[x] = std::max(minimalSizeOnX[x], static_cast<size_t>(fixedSizeValue.x));
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(fixedSizeValue.y));
						break;
					}
					case SizePolicy::Maximum:
					{
						const spk::Vector2UInt maximalSizeValue = element->maximalSize();
						minimalSizeOnX[x] = std::max(minimalSizeOnX[x], static_cast<size_t>(maximalSizeValue.x));
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(maximalSizeValue.y));
						break;
					}
					default:
					{
						const spk::Vector2UInt maximalSizeValue = element->maximalSize();
						const spk::Vector2UInt minimalValue = element->minimalSizeFor(
							{static_cast<unsigned int>(std::min(availableContentWidth, static_cast<size_t>(maximalSizeValue.x))),
							 static_cast<unsigned int>(p_geometry.size.y)});
						minimalSizeOnX[x] = std::max(minimalSizeOnX[x], static_cast<size_t>(minimalValue.x));
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(minimalValue.y));
						break;
					}
					}
				}
			}

			size_t spaceLeftX = (p_geometry.size.x > totalPaddingX) ? p_geometry.size.x - totalPaddingX : 0;

			size_t sumColsMin = 0;
			for (size_t x = 0; x < _size.x; ++x)
			{
				sumColsMin += minimalSizeOnX[x];
			}

			spaceLeftX = (spaceLeftX >= sumColsMin) ? spaceLeftX - sumColsMin : 0;

			const size_t expandValueX = (nbExtendableOnX != 0) ? spaceLeftX / nbExtendableOnX : 0;

			std::vector<size_t> finalSizeOnX(_size.x, 0);

			for (size_t x = 0; x < _size.x; x++)
			{
				finalSizeOnX[x] = minimalSizeOnX[x] + (isExtendableOnX[x] ? expandValueX : 0);
			}

			std::fill(minimalSizeOnY.begin(), minimalSizeOnY.end(), 0);

			for (size_t x = 0; x < _size.x; x++)
			{
				for (size_t y = 0; y < _size.y; y++)
				{
					const auto &element = _elements[_idx(y, x)];

					if (element == nullptr || (element->widget() == nullptr && element->layout() == nullptr))
					{
						continue;
					}

					switch (element->sizePolicy())
					{
					case SizePolicy::Fixed:
					{
						const spk::Vector2UInt fixedSizeValue = element->fixedSize();
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(fixedSizeValue.y));
						break;
					}
					case SizePolicy::Maximum:
					{
						const spk::Vector2UInt maximalSizeValue = element->maximalSize();
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(maximalSizeValue.y));
						break;
					}
					default:
					{
						const size_t effectiveWidth = _effectiveElementWidth(*element, finalSizeOnX[x]);
						const spk::Vector2UInt minimalValue =
							_elementMinimalSizeFor(
								*element,
								{static_cast<unsigned int>(effectiveWidth),
								 static_cast<unsigned int>(p_geometry.size.y)});
						minimalSizeOnY[y] = std::max(minimalSizeOnY[y], static_cast<size_t>(minimalValue.y));
						break;
					}
					}
				}
			}

			size_t spaceLeftY = (p_geometry.size.y > totalPaddingY) ? p_geometry.size.y - totalPaddingY : 0;
			size_t sumRowsMin = 0;
			for (size_t y = 0; y < _size.y; ++y)
			{
				sumRowsMin += minimalSizeOnY[y];
			}

			spaceLeftY = (spaceLeftY >= sumRowsMin) ? spaceLeftY - sumRowsMin : 0;
			const size_t expandValueY = (nbExtendableOnY != 0) ? spaceLeftY / nbExtendableOnY : 0;

			std::vector<size_t> finalSizeOnY(_size.y, 0);

			for (size_t y = 0; y < _size.y; y++)
			{
				finalSizeOnY[y] = minimalSizeOnY[y] + (isExtendableOnY[y] ? expandValueY : 0);
			}

			size_t anchorOnX = 0;

			for (size_t x = 0; x < _size.x; x++)
			{
				size_t anchorOnY = 0;
				for (size_t y = 0; y < _size.y; y++)
				{
					const auto &element = _elements[_idx(y, x)];

					if (element != nullptr)
					{
						const size_t elementWidth = _effectiveElementWidth(*element, finalSizeOnX[x]);
						const size_t elementHeight = _effectiveElementHeight(*element, finalSizeOnY[y], elementWidth);

						element->setGeometry(spk::Rect2D(p_geometry.anchor.x + static_cast<int>(anchorOnX), p_geometry.anchor.y + static_cast<int>(anchorOnY), static_cast<unsigned int>(elementWidth), static_cast<unsigned int>(elementHeight)));
					}
					anchorOnY += finalSizeOnY[y] + _elementPadding.y;
				}
				anchorOnX += finalSizeOnX[x] + _elementPadding.x;
			}
		}
	};

	template <size_t NbColumns>
	class GridLayoutFixedColumns : public GridLayout
	{
	public:
		GridLayoutFixedColumns()
		{
			if (NbColumns == 0)
			{
				return;
			}
			_resizeGrid(0, NbColumns);
		}

		[[nodiscard]] static constexpr size_t columnCount()
		{
			return NbColumns;
		}

		void addEmptyColumn() = delete;

		Element *setWidget(size_t p_column, size_t p_row, spk::Widget *p_widget, SizePolicy p_sizePolicy = SizePolicy::Extend)
		{
			if (p_column >= NbColumns)
			{
				throw std::invalid_argument("GridLayoutFixedColumns::setWidget: invalid column index");
			}
			_ensureSize(p_row + 1, NbColumns);
			return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);
		}
	};

	template <size_t NbRows>
	class GridLayoutFixedRows : public GridLayout
	{
	public:
		GridLayoutFixedRows()
		{
			if (NbRows == 0)
			{
				return;
			}
			_resizeGrid(NbRows, 0);
		}

		[[nodiscard]] static constexpr size_t rowCount()
		{
			return NbRows;
		}

		void addEmptyRow() = delete;

		Element *setWidget(size_t p_column, size_t p_row, spk::Widget *p_widget, SizePolicy p_sizePolicy = SizePolicy::Extend)
		{
			if (p_row >= NbRows)
			{
				throw std::invalid_argument("GridLayoutFixedRows::setWidget: invalid row index");
			}
			_ensureSize(NbRows, p_column + 1);
			return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);
		}
	};

	template <size_t NbColumns, size_t NbRows>
	class GridLayoutFixedGrid : public GridLayout
	{
	public:
		GridLayoutFixedGrid()
		{
			if (NbColumns == 0 || NbRows == 0)
			{
				return;
			}
			_resizeGrid(NbRows, NbColumns);
		}

		[[nodiscard]] static constexpr size_t columnCount()
		{
			return NbColumns;
		}

		[[nodiscard]] static constexpr size_t rowCount()
		{
			return NbRows;
		}

		void addEmptyRow() = delete;
		void addEmptyColumn() = delete;

		Element *setWidget(size_t p_column, size_t p_row, spk::Widget *p_widget, SizePolicy p_sizePolicy = SizePolicy::Extend)
		{
			if (p_column >= NbColumns || p_row >= NbRows)
			{
				throw std::invalid_argument("GridLayoutFixedGrid::setWidget: out-of-bounds");
			}
			return GridLayout::setWidget(p_column, p_row, p_widget, p_sizePolicy);
		}
	};
}
