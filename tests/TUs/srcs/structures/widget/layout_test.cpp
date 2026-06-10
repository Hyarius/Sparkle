#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_form_layout.hpp"
#include "structures/widget/spk_grid_layout.hpp"
#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_widget.hpp"

TEST(LayoutTest, AddWidgetRejectsNull)
{
	spk::HorizontalLayout layout;

	EXPECT_THROW(layout.addWidget(nullptr), std::invalid_argument);
}

TEST(LayoutTest, RemoveWidgetNotContainedThrows)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.removeWidget(&widget), std::runtime_error);
}

TEST(LayoutTest, RemoveWidgetErasesElement)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("Widget");

	layout.addWidget(&widget);
	EXPECT_EQ(layout.elements().size(), 1u);

	layout.removeWidget(&widget);
	EXPECT_TRUE(layout.elements().empty());
}

TEST(HorizontalLayoutTest, DistributesSpaceEvenlyBetweenExtendElements)
{
	spk::HorizontalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");
	spk::Widget third("Third");

	layout.addWidget(&first);
	layout.addWidget(&second);
	layout.addWidget(&third);

	layout.setGeometry(spk::Rect2D(0, 0, 300, 50));

	EXPECT_EQ(first.geometry(), spk::Rect2D(0, 0, 100, 50));
	EXPECT_EQ(second.geometry(), spk::Rect2D(100, 0, 100, 50));
	EXPECT_EQ(third.geometry(), spk::Rect2D(200, 0, 100, 50));
}

TEST(HorizontalLayoutTest, RespectsElementPadding)
{
	spk::HorizontalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	layout.addWidget(&first);
	layout.addWidget(&second);
	layout.setElementPadding({10, 0});

	layout.setGeometry(spk::Rect2D(0, 0, 210, 50));

	EXPECT_EQ(first.geometry(), spk::Rect2D(0, 0, 100, 50));
	EXPECT_EQ(second.geometry(), spk::Rect2D(110, 0, 100, 50));
}

TEST(HorizontalLayoutTest, MinimumPolicyUsesWidgetMinimalSize)
{
	spk::HorizontalLayout layout;
	spk::Widget fixedWidget("Fixed");
	spk::Widget extendWidget("Extend");

	fixedWidget.setMinimalSize({40, 20});

	layout.addWidget(&fixedWidget, spk::Layout::SizePolicy::Minimum);
	layout.addWidget(&extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200, 50));

	EXPECT_EQ(fixedWidget.geometry().width(), 40u);
	EXPECT_EQ(extendWidget.geometry().width(), 160u);
}

TEST(HorizontalLayoutTest, MinimalSizeSumsPrimaryAxis)
{
	spk::HorizontalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	first.setMinimalSize({40, 30});
	second.setMinimalSize({60, 20});

	layout.addWidget(&first);
	layout.addWidget(&second);
	layout.setElementPadding({10, 10});

	EXPECT_EQ(layout.minimalSize(), spk::Vector2UInt(110, 30));
}

TEST(VerticalLayoutTest, StacksElementsVertically)
{
	spk::VerticalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	layout.addWidget(&first);
	layout.addWidget(&second);

	layout.setGeometry(spk::Rect2D(0, 0, 100, 200));

	EXPECT_EQ(first.geometry(), spk::Rect2D(0, 0, 100, 100));
	EXPECT_EQ(second.geometry(), spk::Rect2D(0, 100, 100, 100));
}

TEST(VerticalLayoutTest, MinimalSizeSumsHeights)
{
	spk::VerticalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	first.setMinimalSize({40, 30});
	second.setMinimalSize({60, 20});

	layout.addWidget(&first);
	layout.addWidget(&second);

	EXPECT_EQ(layout.minimalSize(), spk::Vector2UInt(60, 50));
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

TEST(GridLayoutFixedTest, OutOfBoundsColumnThrows)
{
	spk::GridLayoutFixedColumns<2> layout;
	spk::Widget widget("Widget");

	EXPECT_THROW(layout.setWidget(2, 0, &widget, spk::Layout::SizePolicy::Extend), std::invalid_argument);
}

TEST(FormLayoutTest, AddRowCreatesTwoElements)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");

	const spk::FormLayout::FormElement row = layout.addRow(&label, &field);

	EXPECT_EQ(layout.nbRow(), 1u);
	EXPECT_NE(row.labelElement, nullptr);
	EXPECT_NE(row.fieldElement, nullptr);
}

TEST(FormLayoutTest, GeometryAlignsLabelAndFieldColumns)
{
	spk::FormLayout layout;
	spk::Widget firstLabel("FirstLabel");
	spk::Widget firstField("FirstField");
	spk::Widget secondLabel("SecondLabel");
	spk::Widget secondField("SecondField");

	firstLabel.setMinimalSize({50, 20});
	secondLabel.setMinimalSize({30, 20});
	firstField.setMinimalSize({10, 20});
	secondField.setMinimalSize({10, 20});

	layout.addRow(&firstLabel, &firstField);
	layout.addRow(&secondLabel, &secondField);

	layout.setGeometry(spk::Rect2D(0, 0, 200, 100));

	EXPECT_EQ(firstLabel.geometry().x(), 0);
	EXPECT_EQ(secondLabel.geometry().x(), 0);
	EXPECT_EQ(firstLabel.geometry().width(), 50u);
	EXPECT_EQ(secondLabel.geometry().width(), 50u);
	EXPECT_EQ(firstField.geometry().x(), secondField.geometry().x());
	EXPECT_EQ(firstField.geometry().x(), 50);
	EXPECT_EQ(firstField.geometry().width(), 150u);
}

TEST(FormLayoutTest, MinimalSizeCombinesColumns)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");

	label.setMinimalSize({50, 20});
	field.setMinimalSize({80, 30});

	layout.addRow(&label, &field);
	layout.setElementPadding({10, 5});

	EXPECT_EQ(layout.minimalSize(), spk::Vector2UInt(140, 30));
}

TEST(FormLayoutTest, RemoveRowErasesBothElements)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");

	const spk::FormLayout::FormElement row = layout.addRow(&label, &field);
	layout.removeRow(row);

	EXPECT_EQ(layout.nbRow(), 0u);
}

// ─── Layout: addLayout / removeLayout / removeElement / clear ────────────────

TEST(LayoutTest, AddLayoutRejectsNull)
{
	spk::HorizontalLayout layout;

	EXPECT_THROW(layout.addLayout(nullptr), std::invalid_argument);
}

TEST(LayoutTest, AddLayoutAddsElement)
{
	spk::HorizontalLayout outer;
	spk::HorizontalLayout inner;

	outer.addLayout(&inner);

	EXPECT_EQ(outer.elements().size(), 1u);
}

TEST(LayoutTest, RemoveLayoutNotContainedThrows)
{
	spk::HorizontalLayout outer;
	spk::HorizontalLayout inner;

	EXPECT_THROW(outer.removeLayout(&inner), std::runtime_error);
}

TEST(LayoutTest, RemoveLayoutErasesElement)
{
	spk::HorizontalLayout outer;
	spk::HorizontalLayout inner;

	outer.addLayout(&inner);
	outer.removeLayout(&inner);

	EXPECT_TRUE(outer.elements().empty());
}

TEST(LayoutTest, RemoveWidgetNullNoOp)
{
	spk::HorizontalLayout layout;

	EXPECT_NO_THROW(layout.removeWidget(nullptr));
}

TEST(LayoutTest, RemoveElementNullNoOp)
{
	spk::HorizontalLayout layout;

	EXPECT_NO_THROW(layout.removeElement(nullptr));
}

TEST(LayoutTest, RemoveElementWithWidgetElement)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("W");

	spk::Layout::Element* elem = layout.addWidget(&widget);
	EXPECT_EQ(layout.elements().size(), 1u);

	layout.removeElement(elem);

	EXPECT_TRUE(layout.elements().empty());
}

TEST(LayoutTest, RemoveElementWithLayoutElement)
{
	spk::HorizontalLayout outer;
	spk::HorizontalLayout inner;

	spk::Layout::Element* elem = outer.addLayout(&inner);
	EXPECT_EQ(outer.elements().size(), 1u);

	outer.removeElement(elem);

	EXPECT_TRUE(outer.elements().empty());
}

TEST(LayoutTest, ClearEmptiesElements)
{
	spk::HorizontalLayout layout;
	spk::Widget widget("W");

	layout.addWidget(&widget);
	EXPECT_EQ(layout.elements().size(), 1u);

	layout.clear();

	EXPECT_TRUE(layout.elements().empty());
}

TEST(LayoutTest, ElementPaddingGetterReturnsSetValue)
{
	spk::HorizontalLayout layout;

	layout.setElementPadding({5u, 3u});

	EXPECT_EQ(layout.elementPadding(), spk::Vector2UInt(5u, 3u));
}

// ─── Layout::Element: construction and accessors ─────────────────────────────

TEST(LayoutElementTest, DefaultConstructorHasNoWidgetOrLayout)
{
	spk::Layout::Element elem;

	EXPECT_EQ(elem.widget(), nullptr);
	EXPECT_EQ(elem.layout(), nullptr);
	EXPECT_FALSE(elem.isWidget());
	EXPECT_FALSE(elem.isLayout());
}

TEST(LayoutElementTest, WidgetConstructorSetsWidget)
{
	spk::Widget w("W");
	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Extend, {0u, 0u});

	EXPECT_EQ(elem.widget(), &w);
	EXPECT_EQ(elem.layout(), nullptr);
	EXPECT_TRUE(elem.isWidget());
	EXPECT_FALSE(elem.isLayout());
}

TEST(LayoutElementTest, LayoutConstructorSetsLayout)
{
	spk::HorizontalLayout inner;
	spk::Layout::Element elem(&inner, spk::Layout::SizePolicy::Extend, {0u, 0u});

	EXPECT_EQ(elem.widget(), nullptr);
	EXPECT_EQ(elem.layout(), &inner);
	EXPECT_FALSE(elem.isWidget());
	EXPECT_TRUE(elem.isLayout());
}

TEST(LayoutElementTest, SetSizePolicyAndGet)
{
	spk::Widget w("W");
	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Extend, {0u, 0u});

	elem.setSizePolicy(spk::Layout::SizePolicy::Minimum);

	EXPECT_EQ(elem.sizePolicy(), spk::Layout::SizePolicy::Minimum);
}

TEST(LayoutElementTest, SetSizeAndGet)
{
	spk::Widget w("W");
	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Extend, {0u, 0u});

	elem.setSize({42u, 17u});

	EXPECT_EQ(elem.size(), spk::Vector2UInt(42u, 17u));
}

TEST(LayoutElementTest, MinimalSizeFromWidget)
{
	spk::Widget w("W");
	w.setMinimalSize({30u, 20u});

	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Extend, {0u, 0u});

	EXPECT_EQ(elem.minimalSize(), spk::Vector2UInt(30u, 20u));
}

TEST(LayoutElementTest, MinimalSizeFromNestedLayout)
{
	spk::HorizontalLayout inner;
	spk::Widget child("Child");
	child.setMinimalSize({50u, 25u});
	inner.addWidget(&child);

	spk::Layout::Element elem(&inner, spk::Layout::SizePolicy::Extend, {0u, 0u});

	EXPECT_EQ(elem.minimalSize(), spk::Vector2UInt(50u, 25u));
}

TEST(LayoutElementTest, FixedSizeFromWidget)
{
	spk::Widget w("W");
	w.setFixedSize({60u, 35u});

	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Fixed, {0u, 0u});

	EXPECT_EQ(elem.fixedSize(), spk::Vector2UInt(60u, 35u));
}

TEST(LayoutElementTest, MaximalSizeFromWidget)
{
	spk::Widget w("W");
	w.setMaximalSize({100u, 80u});

	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Maximum, {0u, 0u});

	EXPECT_EQ(elem.maximalSize(), spk::Vector2UInt(100u, 80u));
}

TEST(LayoutElementTest, SetGeometryForwardsToWidget)
{
	spk::Widget w("W");
	spk::Layout::Element elem(&w, spk::Layout::SizePolicy::Extend, {0u, 0u});

	elem.setGeometry(spk::Rect2D(10, 20, 80u, 40u));

	EXPECT_EQ(w.geometry(), spk::Rect2D(10, 20, 80u, 40u));
}

TEST(LayoutElementTest, SetGeometryForwardsToNestedLayout)
{
	spk::HorizontalLayout inner;
	spk::Widget child("Child");
	inner.addWidget(&child);

	spk::Layout::Element elem(&inner, spk::Layout::SizePolicy::Extend, {0u, 0u});

	elem.setGeometry(spk::Rect2D(5, 15, 120u, 60u));

	EXPECT_EQ(child.geometry(), spk::Rect2D(5, 15, 120u, 60u));
}

// ─── GridLayout: additional coverage ─────────────────────────────────────────

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

// ─── LinearLayout: additional coverage ───────────────────────────────────────

TEST(HorizontalLayoutTest, EmptyLayoutSetGeometryNoOp)
{
	spk::HorizontalLayout layout;

	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u)));
}

TEST(HorizontalLayoutTest, FixedPolicyUsesElementRequestedSize)
{
	spk::HorizontalLayout layout;
	spk::Widget fixedWidget("Fixed");
	spk::Widget extendWidget("Extend");

	spk::Layout::Element* elem = layout.addWidget(&fixedWidget, spk::Layout::SizePolicy::Fixed);
	elem->setSize({60u, 0u});
	layout.addWidget(&extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	EXPECT_EQ(fixedWidget.geometry().width(), 60u);
	EXPECT_EQ(extendWidget.geometry().width(), 140u);
}

TEST(HorizontalLayoutTest, MaximumPolicyUsesWidgetMaximalSize)
{
	spk::HorizontalLayout layout;
	spk::Widget cappedWidget("Capped");
	spk::Widget extendWidget("Extend");

	cappedWidget.setMaximalSize({80u, 50u});

	layout.addWidget(&cappedWidget, spk::Layout::SizePolicy::Maximum);
	layout.addWidget(&extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	EXPECT_EQ(cappedWidget.geometry().width(), 80u);
	EXPECT_EQ(extendWidget.geometry().width(), 120u);
}

TEST(HorizontalLayoutTest, HorizontalExtendExtendsInHorizontalLayout)
{
	spk::HorizontalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	layout.addWidget(&first, spk::Layout::SizePolicy::HorizontalExtend);
	layout.addWidget(&second, spk::Layout::SizePolicy::HorizontalExtend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	EXPECT_EQ(first.geometry().width(), 100u);
	EXPECT_EQ(second.geometry().width(), 100u);
}

TEST(HorizontalLayoutTest, VerticalExtendUsesMinimalWidthInHorizontalLayout)
{
	spk::HorizontalLayout layout;
	spk::Widget constrained("Constrained");
	spk::Widget extendWidget("Extend");

	constrained.setMinimalSize({50u, 20u});

	layout.addWidget(&constrained, spk::Layout::SizePolicy::VerticalExtend);
	layout.addWidget(&extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	EXPECT_EQ(constrained.geometry().width(), 50u);
	EXPECT_EQ(extendWidget.geometry().width(), 150u);
}

TEST(HorizontalLayoutTest, NonExtendElementsShareExtraSpaceWhenNoExtendPresent)
{
	spk::HorizontalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	first.setMinimalSize({40u, 20u});
	second.setMinimalSize({60u, 20u});

	layout.addWidget(&first, spk::Layout::SizePolicy::Minimum);
	layout.addWidget(&second, spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 50u));

	// Extra = 200 - (40 + 60) = 100, split equally: +50 each
	EXPECT_EQ(first.geometry().width(), 90u);
	EXPECT_EQ(second.geometry().width(), 110u);
}

TEST(VerticalLayoutTest, EmptyLayoutSetGeometryNoOp)
{
	spk::VerticalLayout layout;

	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 100u, 200u)));
}

TEST(VerticalLayoutTest, VerticalExtendExtendsInVerticalLayout)
{
	spk::VerticalLayout layout;
	spk::Widget first("First");
	spk::Widget second("Second");

	layout.addWidget(&first, spk::Layout::SizePolicy::VerticalExtend);
	layout.addWidget(&second, spk::Layout::SizePolicy::VerticalExtend);

	layout.setGeometry(spk::Rect2D(0, 0, 100u, 200u));

	EXPECT_EQ(first.geometry().height(), 100u);
	EXPECT_EQ(second.geometry().height(), 100u);
}

TEST(VerticalLayoutTest, HorizontalExtendUsesMinimalHeightInVerticalLayout)
{
	spk::VerticalLayout layout;
	spk::Widget constrained("Constrained");
	spk::Widget extendWidget("Extend");

	constrained.setMinimalSize({20u, 40u});

	layout.addWidget(&constrained, spk::Layout::SizePolicy::HorizontalExtend);
	layout.addWidget(&extendWidget, spk::Layout::SizePolicy::Extend);

	layout.setGeometry(spk::Rect2D(0, 0, 100u, 200u));

	EXPECT_EQ(constrained.geometry().height(), 40u);
	EXPECT_EQ(extendWidget.geometry().height(), 160u);
}

// ─── FormLayout: additional coverage ─────────────────────────────────────────

TEST(FormLayoutTest, EmptyLayoutSetGeometryNoOp)
{
	spk::FormLayout layout;

	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u)));
}

TEST(FormLayoutTest, MultipleRowsArePlacedSequentially)
{
	spk::FormLayout layout;
	spk::Widget label1("L1");
	spk::Widget field1("F1");
	spk::Widget label2("L2");
	spk::Widget field2("F2");

	label1.setMinimalSize({50u, 20u});
	field1.setMinimalSize({50u, 20u});
	label2.setMinimalSize({50u, 20u});
	field2.setMinimalSize({50u, 20u});

	layout.addRow(&label1, &field1);
	layout.addRow(&label2, &field2);

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 40u));

	EXPECT_EQ(label1.geometry().y(), 0);
	EXPECT_EQ(label2.geometry().y(), 20);
}

TEST(FormLayoutTest, RowPaddingShiftsSubsequentRows)
{
	spk::FormLayout layout;
	spk::Widget label1("L1");
	spk::Widget field1("F1");
	spk::Widget label2("L2");
	spk::Widget field2("F2");

	label1.setMinimalSize({50u, 20u});
	field1.setMinimalSize({50u, 20u});
	label2.setMinimalSize({50u, 20u});
	field2.setMinimalSize({50u, 20u});

	layout.addRow(&label1, &field1);
	layout.addRow(&label2, &field2);
	layout.setElementPadding({0u, 8u});

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 48u));

	EXPECT_EQ(label2.geometry().y(), label1.geometry().y() + 20 + 8);
}

TEST(FormLayoutTest, NbRowTracksAddAndRemove)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");

	EXPECT_EQ(layout.nbRow(), 0u);

	const spk::FormLayout::FormElement row = layout.addRow(&label, &field);
	EXPECT_EQ(layout.nbRow(), 1u);

	layout.removeRow(row);
	EXPECT_EQ(layout.nbRow(), 0u);
}
