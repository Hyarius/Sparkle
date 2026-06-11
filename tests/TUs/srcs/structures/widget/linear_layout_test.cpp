#include <gtest/gtest.h>

#include "structures/widget/spk_linear_layout.hpp"
#include "structures/widget/spk_widget.hpp"

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

	EXPECT_EQ(first.geometry().width(), 90u);
	EXPECT_EQ(second.geometry().width(), 110u);
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
