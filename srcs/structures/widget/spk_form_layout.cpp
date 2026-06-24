#include "structures/widget/spk_form_layout.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

namespace
{
	[[nodiscard]] constexpr uint32_t unlimitedSize()
	{
		return std::numeric_limits<spk::Vector2UInt::value_type>::max();
	}

	[[nodiscard]] bool hasRenderable(const spk::FormLayout::Element *p_element)
	{
		return (p_element != nullptr) && (p_element->widget() != nullptr || p_element->layout() != nullptr);
	}

	[[nodiscard]] uint32_t clampToUInt(size_t p_value)
	{
		return static_cast<uint32_t>(std::min(p_value, static_cast<size_t>(unlimitedSize())));
	}

	[[nodiscard]] spk::Vector2UInt availableForElement(
		const spk::FormLayout::Element *p_element,
		uint32_t p_availableWidth,
		uint32_t p_availableHeight)
	{
		const spk::Vector2UInt maximalSize = p_element->maximalSize();

		return {
			std::min(p_availableWidth, maximalSize.x),
			std::min(p_availableHeight, maximalSize.y)};
	}

	[[nodiscard]] spk::Vector2UInt minimalSizeForElement(
		const spk::FormLayout::Element *p_element,
		uint32_t p_availableWidth,
		uint32_t p_availableHeight)
	{
		if (hasRenderable(p_element) == false)
		{
			return {0U, 0U};
		}

		return p_element->minimalSizeFor(availableForElement(p_element, p_availableWidth, p_availableHeight));
	}

	[[nodiscard]] uint32_t widthForPolicy(
		const spk::FormLayout::Element *p_element,
		uint32_t p_availableWidth,
		uint32_t p_availableHeight)
	{
		if (hasRenderable(p_element) == false)
		{
			return 0U;
		}

		switch (p_element->sizePolicy())
		{
		case spk::FormLayout::SizePolicy::Fixed:
			return p_element->size().x;
		case spk::FormLayout::SizePolicy::Maximum:
			return p_element->maximalSize().x;
		case spk::FormLayout::SizePolicy::Minimum:
		case spk::FormLayout::SizePolicy::HorizontalExtend:
		case spk::FormLayout::SizePolicy::Extend:
		case spk::FormLayout::SizePolicy::VerticalExtend:
			return minimalSizeForElement(p_element, p_availableWidth, p_availableHeight).x;
		}
		return 0U;
	}

	[[nodiscard]] uint32_t heightForPolicy(
		const spk::FormLayout::Element *p_element,
		uint32_t p_availableWidth,
		uint32_t p_availableHeight)
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
			return minimalSizeForElement(p_element, p_availableWidth, p_availableHeight).y;
		}
		return 0;
	}

	[[nodiscard]] bool elementHorizontallyExpandable(const spk::FormLayout::Element *p_element)
	{
		if (p_element == nullptr)
		{
			return false;
		}

		const auto policy = p_element->sizePolicy();
		return (policy == spk::FormLayout::SizePolicy::Extend || policy == spk::FormLayout::SizePolicy::HorizontalExtend);
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
		bool hasLabelColumn = false;
		bool hasFieldColumn = false;
		std::vector<int> rowHeights;
		std::vector<bool> rowExpandable;
	};

	[[nodiscard]] uint32_t columnPadding(const ScanResult &p_scan, const spk::Vector2UInt &p_padding)
	{
		return (p_scan.hasLabelColumn == true && p_scan.hasFieldColumn == true) ? p_padding.x : 0U;
	}

	[[nodiscard]] ScanResult scanColumns(
		const spk::FormLayout &p_layout,
		size_t p_rowCount,
		const spk::Vector2UInt &p_availableSize)
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
			const uint32_t paddingX = (labelRenderable == true && fieldRenderable == true) ? p_layout.elementPadding().x : 0U;

			if (labelRenderable == true)
			{
				result.hasLabelColumn = true;

				const uint32_t labelWidth = widthForPolicy(label, p_availableSize.x, p_availableSize.y);
				result.labelColumnWidth = std::max(result.labelColumnWidth, labelWidth);

				if (elementHorizontallyExpandable(label) == true && fieldRenderable == true)
				{
					result.labelColumnExpandable = true;
				}
			}

			if (fieldRenderable == true)
			{
				result.hasFieldColumn = true;

				uint32_t fieldAvailableWidth = p_availableSize.x;
				if (labelRenderable == true)
				{
					const size_t usedWidth = static_cast<size_t>(widthForPolicy(label, p_availableSize.x, p_availableSize.y)) + paddingX;
					fieldAvailableWidth =
						(p_availableSize.x > usedWidth) ? static_cast<uint32_t>(p_availableSize.x - usedWidth) : 0U;
				}

				const uint32_t fieldWidth = widthForPolicy(field, fieldAvailableWidth, p_availableSize.y);
				result.fieldColumnWidth = std::max(result.fieldColumnWidth, fieldWidth);

				if (elementHorizontallyExpandable(field) == true && labelRenderable == true)
				{
					result.fieldColumnExpandable = true;
				}
			}

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

	void refreshRowHeights(
		const spk::FormLayout &p_layout,
		const spk::Vector2UInt &p_availableSize,
		uint32_t p_labelWidth,
		uint32_t p_fieldWidth,
		std::vector<int> &p_rowHeights)
	{
		const size_t rowCount = p_rowHeights.size();

		for (size_t row = 0; row < rowCount; ++row)
		{
			auto *label = p_layout.elements()[2 * row].get();
			auto *field = p_layout.elements()[2 * row + 1].get();
			const bool labelRenderable = hasRenderable(label);
			const bool fieldRenderable = hasRenderable(field);

			uint32_t labelAvailableWidth = p_labelWidth;
			uint32_t fieldAvailableWidth = p_fieldWidth;

			if (labelRenderable == true && fieldRenderable == false)
			{
				labelAvailableWidth = p_availableSize.x;
			}
			else if (fieldRenderable == true && labelRenderable == false)
			{
				fieldAvailableWidth = p_availableSize.x;
			}

			const uint32_t labelHeight =
				labelRenderable ? heightForPolicy(label, labelAvailableWidth, p_availableSize.y) : 0U;
			const uint32_t fieldHeight =
				fieldRenderable ? heightForPolicy(field, fieldAvailableWidth, p_availableSize.y) : 0U;

			p_rowHeights[row] = static_cast<int>(std::max(labelHeight, fieldHeight));
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
		configureMinimalSizeGenerator([this]() {
			return _computeMinimalSize();
		});
		configureFixedSizeGenerator([this]() {
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

		ScanResult scan = scanColumns(*this, rowCount, p_geometry.size);

		uint32_t labelWidth = scan.labelColumnWidth;
		uint32_t fieldWidth = scan.fieldColumnWidth;

		const uint32_t minTotalWidth = labelWidth + fieldWidth + columnPadding(scan, _elementPadding);

		const uint32_t extraWidth = (p_geometry.size.x > minTotalWidth) ? (p_geometry.size.x - minTotalWidth) : 0U;

		ensureSomethingExpands(scan.labelColumnExpandable, scan.fieldColumnExpandable);
		distributeExtraWidth(extraWidth, scan.labelColumnExpandable, scan.fieldColumnExpandable, labelWidth, fieldWidth);

		refreshRowHeights(*this, p_geometry.size, labelWidth, fieldWidth, scan.rowHeights);

		const uint32_t rowsSum = static_cast<uint32_t>(std::accumulate(scan.rowHeights.begin(), scan.rowHeights.end(), 0));
		const uint32_t gaps = static_cast<uint32_t>((rowCount > 0U ? rowCount - 1U : 0U) * _elementPadding.y);
		const uint32_t minTotalHeight = rowsSum + gaps;
		const uint32_t extraHeight = (p_geometry.size.y > minTotalHeight) ? (p_geometry.size.y - minTotalHeight) : 0U;

		ensureSomeRowsExpand(scan.rowExpandable);
		distributeExtraHeight(extraHeight, scan.rowExpandable, scan.rowHeights);

		placeAllCells(*this, p_geometry, labelWidth, fieldWidth, scan.rowHeights);
	}

	spk::Vector2UInt FormLayout::_computeMinimalSize() const
	{
		return minimalSizeFor(std::numeric_limits<spk::Vector2UInt>::max());
	}

	spk::Vector2UInt FormLayout::minimalSizeFor(const spk::Vector2UInt &p_availableSize) const
	{
		const size_t rowCountValue = nbRow();
		if (rowCountValue == 0U)
		{
			return {0U, 0U};
		}

		ScanResult scan = scanColumns(*this, rowCountValue, p_availableSize);

		uint32_t labelWidth = scan.labelColumnWidth;
		uint32_t fieldWidth = scan.fieldColumnWidth;

		const uint32_t minimumTotalWidth = labelWidth + fieldWidth + columnPadding(scan, _elementPadding);
		const uint32_t extraWidth =
			(p_availableSize.x > minimumTotalWidth) ? (p_availableSize.x - minimumTotalWidth) : 0U;

		ensureSomethingExpands(scan.labelColumnExpandable, scan.fieldColumnExpandable);
		distributeExtraWidth(extraWidth, scan.labelColumnExpandable, scan.fieldColumnExpandable, labelWidth, fieldWidth);
		refreshRowHeights(*this, p_availableSize, labelWidth, fieldWidth, scan.rowHeights);

		const uint32_t minimumTotalHeight =
			static_cast<uint32_t>(std::accumulate(scan.rowHeights.begin(), scan.rowHeights.end(), 0)) +
			static_cast<uint32_t>((rowCountValue > 0U ? rowCountValue - 1U : 0U) * _elementPadding.y);

		return {minimumTotalWidth, minimumTotalHeight};
	}
}
