#include <gtest/gtest.h>

#include <limits>

#include "structures/widget/spk_resizable_element.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget.hpp"

TEST(ResizableElementSizeTest, DefaultsToZeroMinimalAndUnboundedMaximal)
{
	spk::Widget widget("Widget");

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(
		widget.maximalSize(),
		spk::Vector2UInt(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()));
}

TEST(ResizableElementSizeTest, SetValuesActAsConstantGenerators)
{
	spk::Widget widget("Widget");

	widget.configureMinimalSizeGenerator([&]() -> spk::Vector2UInt {return {100, 200};});
	widget.configureFixedSizeGenerator([&]() -> spk::Vector2UInt {return {200, 400};});
	widget.configureMaximalSizeGenerator([&]() -> spk::Vector2UInt {return {300, 600};});

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(100, 200));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(200, 400));
	EXPECT_EQ(widget.maximalSize(), spk::Vector2UInt(300, 600));

	widget.setMinimalSize({10, 20});
	widget.setFixedSize({30, 40});
	widget.setMaximalSize({50, 60});

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(10, 20));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(30, 40));
	EXPECT_EQ(widget.maximalSize(), spk::Vector2UInt(50, 60));

	// A set value installs a constant generator, so it survives a cache release
	// (releaseSizeCache is triggered on every geometry/render invalidation).
	widget.releaseSizeCache();

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(10, 20));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(30, 40));
	EXPECT_EQ(widget.maximalSize(), spk::Vector2UInt(50, 60));
}

TEST(ResizableElementSizeTest, GeneratorIsCachedUntilRelease)
{
	spk::Widget widget("Widget");

	int callCount = 0;
	widget.configureMinimalSizeGenerator([&callCount]() {
		++callCount;
		return spk::Vector2UInt(static_cast<unsigned int>(callCount), 0);
	});

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(1, 0));
	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(1, 0));
	EXPECT_EQ(callCount, 1);

	widget.releaseMinimalSize();

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(2, 0));
	EXPECT_EQ(callCount, 2);
}

TEST(ResizableElementSizeTest, WidgetSizeSettersConfigureElementSizes)
{
	spk::Widget widget("Widget");

	widget.setMinimalSize({15, 25});
	widget.setFixedSize({35, 45});
	widget.setMaximalSize({55, 65});

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(15, 25));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(35, 45));
	EXPECT_EQ(widget.maximalSize(), spk::Vector2UInt(55, 65));
}

TEST(ResizableElementSizeTest, TextLabelMinimalSizeTracksTextChanges)
{
	spk::TextLabel label("Label", "Hi");

	const spk::Vector2UInt shortSize = label.minimalSize();
	EXPECT_GT(shortSize.x, 0u);

	label.setText("A considerably longer text");

	const spk::Vector2UInt longSize = label.minimalSize();
	EXPECT_GT(longSize.x, shortSize.x);
}

TEST(ResizableElementSizeTest, ChildEditionReleasesParentCachedSizes)
{
	spk::Widget parent("Parent");
	spk::TextLabel child("Child", "Hi", &parent);

	int callCount = 0;
	parent.configureMinimalSizeGenerator([&callCount, &child]() {
		++callCount;
		return child.minimalSize();
	});

	const spk::Vector2UInt firstSize = parent.minimalSize();
	EXPECT_EQ(callCount, 1);

	child.setText("A considerably longer text");

	const spk::Vector2UInt secondSize = parent.minimalSize();
	EXPECT_EQ(callCount, 2);
	EXPECT_GT(secondSize.x, firstSize.x);
}
