#include "structures/widget/spk_form_layout.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace
{
	[[nodiscard]] bool hasRenderable(const spk::FormLayout::Element *p_element)
	{
		return (p_element != nullptr) && (p_element->widget() != nullptr || p_element->layout() != nullptr);
	}

	[[nodiscard]] uint32_t heightForPolicy(const spk::FormLayout::Element *p_element)
	{
		switch (p_element->sizePolicy())
		{
		case spk::FormLayout::SizePolicy::Fixed:
			return p_element->size().y;
		case spk::FormLayout::SizePolicy::Maximum:
			return p_element->maximalSize().y;
		case spk::FormLayout::SizePolicy::Minimum:
		case spk::FormLayout::SizePolicy::HorizontalExtend:
		case spk::FormLayout::SizePolicy::Extend:
		case spk::FormLayout::SizePolicy::VerticalExtend:
			return p_element->minimalSize().y;
		}
		return 0;
	}

	void accumulateColumn(const spk::FormLayout::Element *p_element, uint32_t &p_columnWidth, bool &p_columnExpandable)
	{
		if (hasRenderable(p_element) == false)
		{
			return;
		}

		switch (p_element->sizePolicy())
		{
		case spk::FormLayout::SizePolicy::Fixed:
			p_columnWidth = std::max(p_columnWidth, p_element->size().x);
			break;
		case spk::FormLayout::SizePolicy::Minimum:
		case spk::FormLayout::SizePolicy::VerticalExtend:
			p_columnWidth = std::max(p_columnWidth, p_element->minimalSize().x);
			break;
		case spk::FormLayout::SizePolicy::Maximum:
			p_columnWidth = std::min(p_columnWidth, p_element->maximalSize().x);
			break;
		case spk::FormLayout::SizePolicy::Extend:
		case spk::FormLayout::SizePolicy::HorizontalExtend:
			p_columnExpandable = true;
			break;
		}
	}

	[[nodiscard]] bool elementVerticallyExpandable(const spk::FormLayout::Element *p_element)
	{
		if (p_element == nullptr)
		{
			return false;
		}
		const auto policy = p_element->sizePolicy();
		return (policy == spk::FormLayout::SizePolicy::Extend || policy == spk::FormLayout::SizePolicy::VerticalExtend);
	}

	struct ScanResult
	{
		uint32_t labelColumnWidth = 0;
		uint32_t fieldColumnWidth = 0;
		bool labelColumnExpandable = false;
		bool fieldColumnExpandable = false;
		std::vector<int> rowHeights;
		std::vector<bool> rowExpandable;
	};

	[[nodiscard]] ScanResult scanRowsAndColumns(spk::FormLayout &p_layout, size_t p_rowCount)
	{
		ScanResult result;
		result.rowHeights.resize(p_rowCount, 0);
		result.rowExpandable.resize(p_rowCount, false);

		for (size_t row = 0; row < p_rowCount; ++row)
		{
			auto *label = p_layout.elements()[2 * row].get();
			auto *field = p_layout.elements()[2 * row + 1].get();
			const bool labelRenderable = hasRenderable(label);
			const bool fieldRenderable = hasRenderable(field);

			if (labelRenderable == true)
			{
				bool labelExpandable = false;
				accumulateColumn(label, result.labelColumnWidth, labelExpandable);
				if (labelExpandable == true && fieldRenderable == true)
				{
					result.labelColumnExpandable = true;
				}
			}

			if (fieldRenderable == true)
			{
				bool fieldExpandable = false;
				accumulateColumn(field, result.fieldColumnWidth, fieldExpandable);
				if (fieldExpandable == true && labelRenderable == true)
				{
					result.fieldColumnExpandable = true;
				}
			}

			const uint32_t labelHeight = labelRenderable ? heightForPolicy(label) : 0U;
			const uint32_t fieldHeight = fieldRenderable ? heightForPolicy(field) : 0U;
			result.rowHeights[row] = static_cast<int>(std::max(labelHeight, fieldHeight));

			result.rowExpandable[row] = (elementVerticallyExpandable(label) || elementVerticallyExpandable(field));
		}

		return result;
	}

	void ensureSomethingExpands(bool &p_labelExpandable, bool &p_fieldExpandable)
	{
		if (p_labelExpandable == false && p_fieldExpandable == false)
		{
			p_labelExpandable = true;
			p_fieldExpandable = true;
		}
	}

	void ensureSomeRowsExpand(std::vector<bool> &p_rowExpandable)
	{
		const bool any = std::any_of(p_rowExpandable.begin(), p_rowExpandable.end(), [](bool p_value) {
			return p_value;
		});
		if (any == false)
		{
			std::fill(p_rowExpandable.begin(), p_rowExpandable.end(), true);
		}
	}

	void distributeExtraWidth(
		uint32_t p_extraWidth,
		bool p_labelExpandable,
		bool p_fieldExpandable,
		uint32_t &p_labelWidth,
		uint32_t &p_fieldWidth)
	{
		const uint32_t expandables = static_cast<uint32_t>(p_labelExpandable) + static_cast<uint32_t>(p_fieldExpandable);
		if (expandables == 0U || p_extraWidth == 0U)
		{
			return;
		}

		const uint32_t share = p_extraWidth / expandables;
		uint32_t remainder = p_extraWidth % expandables;

		if (p_labelExpandable == true)
		{
			p_labelWidth += share + ((remainder-- > 0U) ? 1U : 0U);
		}
		if (p_fieldExpandable == true)
		{
			p_fieldWidth += share + ((remainder-- > 0U) ? 1U : 0U);
		}
	}

	void distributeExtraHeight(uint32_t p_extraHeight, const std::vector<bool> &p_rowExpandable, std::vector<int> &p_rowHeights)
	{
		if (p_extraHeight == 0U)
		{
			return;
		}

		const uint32_t count = static_cast<uint32_t>(std::count(p_rowExpandable.begin(), p_rowExpandable.end(), true));
		if (count == 0U)
		{
			return;
		}

		const uint32_t share = p_extraHeight / count;
		uint32_t remainder = p_extraHeight % count;

		for (size_t i = 0; i < p_rowHeights.size(); ++i)
		{
			if (p_rowExpandable[i] == true)
			{
				p_rowHeights[i] += static_cast<int>(share + ((remainder-- > 0U) ? 1U : 0U));
			}
		}
	}

	void placeAllCells(
		spk::FormLayout &p_layout,
		const spk::Rect2D &p_geometry,
		uint32_t p_labelWidth,
		uint32_t p_fieldWidth,
		const std::vector<int> &p_rowHeights)
	{
		int y = p_geometry.anchor.y;
		const uint32_t paddingX = p_layout.elementPadding().x;
		const uint32_t rowWidth = p_geometry.size.x;

		for (size_t row = 0; row < p_rowHeights.size(); ++row)
		{
			const uint32_t height = static_cast<uint32_t>(p_rowHeights[row]);

			auto *label = p_layout.elements()[2 * row].get();
			auto *field = p_layout.elements()[2 * row + 1].get();
			const bool labelRenderable = hasRenderable(label);
			const bool fieldRenderable = hasRenderable(field);

			if (labelRenderable == true && fieldRenderable == true)
			{
				label->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {p_labelWidth, height}));
				field->setGeometry(spk::Rect2D({p_geometry.anchor.x + static_cast<int>(p_labelWidth + paddingX), y}, {p_fieldWidth, height}));
			}
			else if (labelRenderable == true)
			{
				label->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height}));
			}
			else if (fieldRenderable == true)
			{
				field->setGeometry(spk::Rect2D({p_geometry.anchor.x, y}, {rowWidth, height}));
			}

			y += static_cast<int>(height + p_layout.elementPadding().y);
		}
	}
}

namespace spk
{
	FormLayout::FormLayout()
	{
		sizeHint().configureMinimalGenerator([this]() {
			return _computeMinimalSize();
		});
		sizeHint().configureDesiredGenerator([this]() {
			return _computeMinimalSize();
		});
	}

	FormLayout::FormElement FormLayout::addRow(
		spk::Widget *p_labelWidget,
		spk::Widget *p_fieldWidget,
		SizePolicy p_labelPolicy,
		SizePolicy p_fieldPolicy)
	{
		return FormElement{addWidget(p_labelWidget, p_labelPolicy), addWidget(p_fieldWidget, p_fieldPolicy)};
	}

	void FormLayout::removeRow(const FormElement &p_row)
	{
		removeElement(p_row.labelElement);
		removeElement(p_row.fieldElement);
	}

	size_t FormLayout::nbRow() const
	{
		return _elements.size() / 2;
	}

	void FormLayout::setGeometry(const spk::Rect2D &p_geometry)
	{
		const size_t rowCount = nbRow();
		if (rowCount == 0U)
		{
			return;
		}

		ScanResult scan = scanRowsAndColumns(*this, rowCount);

		uint32_t labelWidth = scan.labelColumnWidth;
		uint32_t fieldWidth = scan.fieldColumnWidth;

		const uint32_t minTotalWidth = labelWidth + fieldWidth + _elementPadding.x;
		const uint32_t rowsSum = static_cast<uint32_t>(std::accumulate(scan.rowHeights.begin(), scan.rowHeights.end(), 0));
		const uint32_t gaps = static_cast<uint32_t>((rowCount > 0U ? rowCount - 1U : 0U) * _elementPadding.y);
		const uint32_t minTotalHeight = rowsSum + gaps;

		const uint32_t extraWidth = (p_geometry.size.x > minTotalWidth) ? (p_geometry.size.x - minTotalWidth) : 0U;
		const uint32_t extraHeight = (p_geometry.size.y > minTotalHeight) ? (p_geometry.size.y - minTotalHeight) : 0U;

		ensureSomethingExpands(scan.labelColumnExpandable, scan.fieldColumnExpandable);
		distributeExtraWidth(extraWidth, scan.labelColumnExpandable, scan.fieldColumnExpandable, labelWidth, fieldWidth);

		ensureSomeRowsExpand(scan.rowExpandable);
		distributeExtraHeight(extraHeight, scan.rowExpandable, scan.rowHeights);

		placeAllCells(*this, p_geometry, labelWidth, fieldWidth, scan.rowHeights);
	}

	spk::Vector2UInt FormLayout::_computeMinimalSize() const
	{
		const size_t rowCountValue = nbRow();
		if (rowCountValue == 0U)
		{
			return {0U, 0U};
		}

		uint32_t labelColumnWidth = 0U;
		uint32_t fieldColumnWidth = 0U;
		std::vector<uint32_t> rowHeights(rowCountValue, 0U);

		const auto elementMinWidth = [](Element *p_element) -> uint32_t {
			return (hasRenderable(p_element) ? p_element->minimalSize().x : 0U);
		};
		const auto elementMinHeight = [](Element *p_element) -> uint32_t {
			return (hasRenderable(p_element) ? p_element->minimalSize().y : 0U);
		};

		for (size_t row = 0; row < rowCountValue; ++row)
		{
			Element *labelElement = _elements[2 * row].get();
			Element *fieldElement = _elements[2 * row + 1].get();

			labelColumnWidth = std::max(labelColumnWidth, elementMinWidth(labelElement));
			fieldColumnWidth = std::max(fieldColumnWidth, elementMinWidth(fieldElement));
			rowHeights[row] = std::max(elementMinHeight(labelElement), elementMinHeight(fieldElement));
		}

		const uint32_t minimumTotalWidth = labelColumnWidth + fieldColumnWidth + _elementPadding.x;
		const uint32_t minimumTotalHeight =
			static_cast<uint32_t>(std::accumulate(rowHeights.begin(), rowHeights.end(), 0U)) +
			static_cast<uint32_t>((rowCountValue > 0U ? rowCountValue - 1U : 0U) * _elementPadding.y);

		return {minimumTotalWidth, minimumTotalHeight};
	}
}
