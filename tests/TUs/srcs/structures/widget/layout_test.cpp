#include <gtest/gtest.h>

#include <stdexcept>

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
