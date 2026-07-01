#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_grid_layout.hpp"
#include "structures/widget/spk_widget.hpp"

namespace
{
	class GridLayoutTester : public spk::GridLayout
	{
	public:
		using spk::GridLayout::_resizeGrid;
	};
}

TEST(GridLayoutTest, SetWidgetGrowsGrid)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");

	layout.setWidget(2, 1, &widget, spk::Layout::SizePolicy::Extend);

	EXPECT_EQ(layout.columnCount(), 3u);
	EXPECT_EQ(layout.rowCount(), 2u);
}

TEST(GridLayoutTest, MinimalSizeUsesColumnAndRowMaxima)
{
	spk::GridLayout layout;
	spk::Widget topLeft("TopLeft");
	spk::Widget bottomRight("BottomRight");

	topLeft.setMinimalSize({40, 30});
	bottomRight.setMinimalSize({60, 20});

	layout.setWidget(0, 0, &topLeft, spk::Layout::SizePolicy::Minimum);
	layout.setWidget(1, 1, &bottomRight, spk::Layout::SizePolicy::Minimum);

	EXPECT_EQ(layout.minimalSize(), spk::Vector2UInt(100, 50));
}

TEST(GridLayoutTest, GeometryPlacesCellsInColumnsAndRows)
{
	spk::GridLayout layout;
	spk::Widget topLeft("TopLeft");
	spk::Widget topRight("TopRight");
	spk::Widget bottomLeft("BottomLeft");
	spk::Widget bottomRight("BottomRight");

	layout.setWidget(0, 0, &topLeft, spk::Layout::SizePolicy::Extend);
	layout.setWidget(1, 0, &topRight, spk::Layout::SizePolicy::Extend);
	layout.setWidget(0, 1, &bottomLeft, spk::Layout::SizePolicy::Extend);
	layout.setWidget(1, 1, &bottomRight, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200, 100));

	EXPECT_EQ(topLeft.geometry(), spk::Rect2D(0, 0, 100, 50));
	EXPECT_EQ(topRight.geometry(), spk::Rect2D(100, 0, 100, 50));
	EXPECT_EQ(bottomLeft.geometry(), spk::Rect2D(0, 50, 100, 50));
	EXPECT_EQ(bottomRight.geometry(), spk::Rect2D(100, 50, 100, 50));
}

TEST(GridLayoutTest, SetWidgetNullThrows)
{
	spk::GridLayout layout;

	EXPECT_THROW(layout.setWidget(0, 0, nullptr), std::invalid_argument);
}

TEST(GridLayoutTest, AddEmptyRowIncreasesRowCount)
{
	spk::GridLayout layout;

	layout.addEmptyRow();

	EXPECT_EQ(layout.rowCount(), 1u);
}

TEST(GridLayoutTest, AddEmptyColumnIncreasesColumnCount)
{
	spk::GridLayout layout;

	layout.addEmptyColumn();

	EXPECT_EQ(layout.columnCount(), 1u);
}

TEST(GridLayoutTest, ClearResetsGridDimensions)
{
	spk::GridLayout layout;
	spk::Widget w("W");
	layout.setWidget(1, 1, &w);

	layout.clear();

	EXPECT_EQ(layout.rowCount(), 0u);
	EXPECT_EQ(layout.columnCount(), 0u);
	EXPECT_TRUE(layout.elements().empty());
}

TEST(GridLayoutTest, SetGeometryEmptyGridNoOp)
{
	spk::GridLayout layout;

	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u)));
}

TEST(GridLayoutTest, SetGeometryColumnPaddingShiftsSecondColumn)
{
	spk::GridLayout layout;
	spk::Widget left("Left");
	spk::Widget right("Right");

	layout.setWidget(0, 0, &left, spk::Layout::SizePolicy::Extend);
	layout.setWidget(1, 0, &right, spk::Layout::SizePolicy::Extend);
	layout.setElementPadding({10u, 0u});

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	EXPECT_EQ(right.geometry().x(), static_cast<int>(left.geometry().width()) + 10);
}

TEST(GridLayoutTest, SetGeometryRowPaddingShiftsSecondRow)
{
	spk::GridLayout layout;
	spk::Widget top("Top");
	spk::Widget bottom("Bottom");

	layout.setWidget(0, 0, &top, spk::Layout::SizePolicy::Extend);
	layout.setWidget(0, 1, &bottom, spk::Layout::SizePolicy::Extend);
	layout.setElementPadding({0u, 10u});

	layout.setGeometry(spk::Rect2D(0, 0, 100u, 200u));

	EXPECT_EQ(bottom.geometry().y(), static_cast<int>(top.geometry().height()) + 10);
}

TEST(GridLayoutTest, HorizontalExtendPolicyExtendsColumnNotRow)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");
	widget.setMinimalSize({40u, 30u});

	layout.setWidget(0, 0, &widget, spk::Layout::SizePolicy::HorizontalExtend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u));

	EXPECT_GT(widget.geometry().width(), 40u);
	EXPECT_EQ(widget.geometry().height(), 30u);
}

TEST(GridLayoutTest, VerticalExtendPolicyExtendsRowNotColumn)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");
	widget.setMinimalSize({40u, 30u});

	layout.setWidget(0, 0, &widget, spk::Layout::SizePolicy::VerticalExtend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u));

	EXPECT_EQ(widget.geometry().width(), 40u);
	EXPECT_GT(widget.geometry().height(), 30u);
}

TEST(GridLayoutTest, FixedPolicyUsesElementFixedSize)
{
	spk::GridLayout layout;
	spk::Widget fixedWidget("Fixed");
	spk::Widget extendWidget("Extend");

	fixedWidget.setFixedSize({60u, 40u});

	layout.setWidget(0, 0, &fixedWidget, spk::Layout::SizePolicy::Fixed);
	layout.setWidget(1, 0, &extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 40u));

	EXPECT_EQ(fixedWidget.geometry().width(), 60u);
}

TEST(GridLayoutTest, MaximumPolicyUsesElementMaximalSize)
{
	spk::GridLayout layout;
	spk::Widget cappedWidget("Capped");
	spk::Widget extendWidget("Extend");

	cappedWidget.setMaximalSize({80u, 40u});

	layout.setWidget(0, 0, &cappedWidget, spk::Layout::SizePolicy::Maximum);
	layout.setWidget(1, 0, &extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 40u));

	EXPECT_EQ(cappedWidget.geometry().width(), 80u);
}

TEST(GridLayoutTest, MaximalSizeCapsMinimumPolicyGeometry)
{
	spk::GridLayout layout;
	spk::Widget wideWidget("Wide");
	spk::Widget cappedWidget("Capped");

	wideWidget.setMinimalSize({200u, 20u});
	cappedWidget.setMinimalSize({40u, 20u});
	cappedWidget.setMaximalSize({80u, 40u});

	layout.setWidget(0, 0, &wideWidget, spk::Layout::SizePolicy::Minimum);
	layout.setWidget(0, 1, &cappedWidget, spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u));

	EXPECT_EQ(wideWidget.geometry().width(), 200u);
	EXPECT_EQ(cappedWidget.geometry().width(), 80u);
}

TEST(GridLayoutFixedTest, OutOfBoundsColumnThrows)
{
	spk::GridLayoutFixedColumns<2> layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.setWidget(2, 0, &widget, spk::Layout::SizePolicy::Extend), std::invalid_argument);
}

TEST(GridLayoutFixedTest, OutOfBoundsRowThrows)
{
	spk::GridLayoutFixedRows<2> layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.setWidget(0, 2, &widget, spk::Layout::SizePolicy::Extend), std::invalid_argument);
}

TEST(GridLayoutFixedTest, FixedGridOutOfBoundsColumnThrows)
{
	spk::GridLayoutFixedGrid<2, 2> layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.setWidget(2, 0, &widget, spk::Layout::SizePolicy::Extend), std::invalid_argument);
}

TEST(GridLayoutFixedTest, FixedGridOutOfBoundsRowThrows)
{
	spk::GridLayoutFixedGrid<2, 2> layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.setWidget(0, 2, &widget, spk::Layout::SizePolicy::Extend), std::invalid_argument);
}

TEST(GridLayoutFixedTest, FixedGridInitialisesWithCorrectDimensions)
{
	spk::GridLayoutFixedGrid<3, 2> layout;

	EXPECT_EQ(layout.columnCount(), 3u);
	EXPECT_EQ(layout.rowCount(), 2u);
}

TEST(GridLayoutTest, PreferredAndFixedSizeOfEmptyGridAreZero)
{
	spk::GridLayout layout;

	EXPECT_EQ(layout.preferredSizeFor({100u, 100u}), spk::Vector2UInt::Zero);
	EXPECT_EQ(layout.fixedSize(), spk::Vector2UInt::Zero);
}

TEST(GridLayoutTest, PreferredSizeHandlesFixedMaximumAndMinimumPolicies)
{
	spk::GridLayout layout;
	spk::Widget fixed("Fixed");
	spk::Widget maximum("Maximum");
	spk::Widget minimum("Minimum");
	fixed.setFixedSize({30u, 20u});
	maximum.setMaximalSize({40u, 25u});
	minimum.setMinimalSize({10u, 15u});
	layout.setWidget(0, 0, &fixed, spk::Layout::SizePolicy::Fixed);
	layout.setWidget(1, 0, &maximum, spk::Layout::SizePolicy::Maximum);
	layout.setWidget(2, 0, &minimum, spk::Layout::SizePolicy::Minimum);
	layout.setElementPadding({5u, 0u});

	EXPECT_EQ(layout.preferredSizeFor({200u, 100u}), spk::Vector2UInt(90u, 25u));
	EXPECT_EQ(layout.fixedSize(), layout.minimalSize());
}

TEST(GridLayoutTest, SettingSameCellAgainReplacesElement)
{
	spk::GridLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	layout.setWidget(0, 0, &first);
	spk::Layout::Element *element = layout.setWidget(0, 0, &second, spk::Layout::SizePolicy::Fixed);

	ASSERT_NE(element, nullptr);
	EXPECT_EQ(element->widget(), &second);
	EXPECT_EQ(element->sizePolicy(), spk::Layout::SizePolicy::Fixed);
	EXPECT_EQ(layout.elements().size(), 1u);
}

TEST(GridLayoutTest, EmptyRowsAndColumnsCanBeAddedToExistingGrid)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");
	layout.setWidget(0, 0, &widget);

	layout.addEmptyRow();
	layout.addEmptyColumn();

	EXPECT_EQ(layout.rowCount(), 2u);
	EXPECT_EQ(layout.columnCount(), 2u);
}

TEST(GridLayoutTest, ResizeToCurrentDimensionsIsANoOp)
{
	GridLayoutTester layout;
	layout._resizeGrid(2u, 3u);

	layout._resizeGrid(2u, 3u);

	EXPECT_EQ(layout.rowCount(), 2u);
	EXPECT_EQ(layout.columnCount(), 3u);
}

TEST(GridLayoutTest, SparseGridGeometrySkipsEmptyCells)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");
	layout.setWidget(1, 1, &widget);

	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 100u, 100u)));
	EXPECT_FALSE(widget.geometry().empty());
}

TEST(GridLayoutTest, GeometrySmallerThanMinimumClampsRemainingSpaceToZero)
{
	spk::GridLayout layout;
	spk::Widget widget("Widget");
	widget.setMinimalSize({100u, 80u});
	layout.setWidget(0, 0, &widget, spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 10u, 5u));

	EXPECT_EQ(widget.geometry().size, spk::Vector2UInt(100u, 80u));
}

TEST(GridLayoutFixedTest, ValidCellsCanBeSetForFixedDimensions)
{
	spk::Widget first("First");
	spk::Widget second("Second");
	spk::Widget third("Third");
	spk::GridLayoutFixedColumns<2> columns;
	spk::GridLayoutFixedRows<2> rows;
	spk::GridLayoutFixedGrid<2, 2> grid;

	EXPECT_NE(columns.setWidget(1, 2, &first), nullptr);
	EXPECT_NE(rows.setWidget(2, 1, &second), nullptr);
	EXPECT_NE(grid.setWidget(1, 1, &third), nullptr);
	EXPECT_EQ(columns.rowCount(), 3u);
	EXPECT_EQ(rows.columnCount(), 3u);
}

TEST(GridLayoutFixedTest, ZeroDimensionSpecializationsStayEmpty)
{
	spk::GridLayoutFixedColumns<0> columns;
	spk::GridLayoutFixedRows<0> rows;
	spk::GridLayoutFixedGrid<0, 2> zeroColumns;
	spk::GridLayoutFixedGrid<2, 0> zeroRows;

	EXPECT_EQ(columns.columnCount(), 0u);
	EXPECT_EQ(rows.rowCount(), 0u);
	EXPECT_EQ(zeroColumns.columnCount(), 0u);
	EXPECT_EQ(zeroRows.rowCount(), 0u);
}
