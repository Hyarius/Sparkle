#include <gtest/gtest.h>

#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_widget.hpp"

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
