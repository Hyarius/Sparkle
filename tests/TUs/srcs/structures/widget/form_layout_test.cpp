#include <gtest/gtest.h>

#include "structures/widget/spk_form_layout.hpp"
#include "structures/widget/spk_widget.hpp"

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

TEST(FormLayoutTest, EmptyLayoutPreferredAndFixedSizesAreZero)
{
	spk::FormLayout layout;

	EXPECT_EQ(layout.preferredSizeFor({200u, 100u}), spk::Vector2UInt::Zero);
	EXPECT_EQ(layout.fixedSize(), spk::Vector2UInt::Zero);
}

TEST(FormLayoutTest, PreferredSizeHandlesFixedAndMaximumPolicies)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");
	label.setFixedSize({40u, 20u});
	field.setMaximalSize({80u, 30u});
	layout.addRow(
		&label,
		&field,
		spk::Layout::SizePolicy::Fixed,
		spk::Layout::SizePolicy::Maximum);
	layout.setElementPadding({5u, 0u});

	EXPECT_EQ(layout.preferredSizeFor({200u, 100u}), spk::Vector2UInt(85u, 30u));
	EXPECT_EQ(layout.fixedSize(), layout.minimalSize());

	layout.setGeometry(spk::Rect2D(0, 0, 200u, 100u));
	EXPECT_FALSE(label.geometry().empty());
	EXPECT_FALSE(field.geometry().empty());
	EXPECT_EQ(label.geometry().width() + field.geometry().width() + 5u, 200u);
}

TEST(FormLayoutTest, NonExpandableColumnsAndRowsShareExtraSpace)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");
	label.setMinimalSize({20u, 10u});
	field.setMinimalSize({30u, 10u});
	layout.addRow(
		&label,
		&field,
		spk::Layout::SizePolicy::Minimum,
		spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 100u, 40u));

	EXPECT_GT(label.geometry().width(), 20u);
	EXPECT_GT(field.geometry().width(), 30u);
	EXPECT_EQ(label.geometry().height(), 40u);
	EXPECT_EQ(field.geometry().height(), 40u);
}

TEST(FormLayoutTest, ExpandableLabelColumnReceivesExtraWidth)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");
	label.setMinimalSize({20u, 10u});
	field.setMinimalSize({30u, 10u});
	layout.addRow(
		&label,
		&field,
		spk::Layout::SizePolicy::HorizontalExtend,
		spk::Layout::SizePolicy::Minimum);

	layout.setGeometry(spk::Rect2D(0, 0, 100u, 20u));

	EXPECT_EQ(field.geometry().width(), 30u);
	EXPECT_EQ(label.geometry().width(), 70u);
}

TEST(FormLayoutTest, LabelOnlyRowUsesFullAvailableWidth)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget discardedField("DiscardedField");
	label.setMinimalSize({25u, 10u});
	layout.addRow(&label, &discardedField);
	layout.elements()[1] = std::make_unique<spk::Layout::Element>();

	EXPECT_EQ(layout.preferredSizeFor({120u, 30u}).x, 25u);
	layout.setGeometry(spk::Rect2D(0, 0, 120u, 30u));
	EXPECT_EQ(label.geometry(), spk::Rect2D(0, 0, 120u, 30u));
}

TEST(FormLayoutTest, FieldOnlyRowUsesFullAvailableWidth)
{
	spk::FormLayout layout;
	spk::Widget discardedLabel("DiscardedLabel");
	spk::Widget field("Field");
	field.setMinimalSize({35u, 10u});
	layout.addRow(&discardedLabel, &field);
	layout.elements()[0].reset();

	EXPECT_EQ(layout.preferredSizeFor({120u, 30u}).x, 35u);
	layout.setGeometry(spk::Rect2D(0, 0, 120u, 30u));
	EXPECT_EQ(field.geometry(), spk::Rect2D(0, 0, 120u, 30u));
}

TEST(FormLayoutTest, EmptyElementRowIsIgnored)
{
	spk::FormLayout layout;
	spk::Widget label("Label");
	spk::Widget field("Field");
	layout.addRow(&label, &field);
	layout.elements()[0].reset();
	layout.elements()[1] = std::make_unique<spk::Layout::Element>();

	EXPECT_EQ(layout.preferredSizeFor({100u, 100u}), spk::Vector2UInt::Zero);
	EXPECT_NO_THROW(layout.setGeometry(spk::Rect2D(0, 0, 100u, 100u)));
}
