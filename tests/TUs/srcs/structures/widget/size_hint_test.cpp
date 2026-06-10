#include <gtest/gtest.h>

#include <limits>

#include "structures/widget/spk_resizable_element.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget.hpp"

TEST(SizeHintTest, DefaultsToZeroMinimalAndUnboundedMaximal)
{
	spk::Widget widget("Widget");

	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(widget.sizeHint().desired(), spk::Vector2UInt(0, 0));
	EXPECT_EQ(
		widget.sizeHint().maximal(),
		spk::Vector2UInt(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()));
}

TEST(SizeHintTest, SetValuesActAsConstantGenerators)
{
	spk::Widget widget("Widget");

	widget.sizeHint().setMinimal({10, 20});
	widget.sizeHint().setDesired({30, 40});
	widget.sizeHint().setMaximal({50, 60});

	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(10, 20));
	EXPECT_EQ(widget.sizeHint().desired(), spk::Vector2UInt(30, 40));
	EXPECT_EQ(widget.sizeHint().maximal(), spk::Vector2UInt(50, 60));

	// Constant values survive a release because they are stored as generators.
	widget.sizeHint().release();

	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(10, 20));
	EXPECT_EQ(widget.sizeHint().maximal(), spk::Vector2UInt(50, 60));
}

TEST(SizeHintTest, GeneratorIsCachedUntilRelease)
{
	spk::Widget widget("Widget");

	int callCount = 0;
	widget.sizeHint().configureMinimalGenerator([&callCount]() {
		++callCount;
		return spk::Vector2UInt(static_cast<unsigned int>(callCount), 0);
	});

	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(1, 0));
	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(1, 0));
	EXPECT_EQ(callCount, 1);

	widget.sizeHint().releaseMinimal();

	EXPECT_EQ(widget.sizeHint().minimal(), spk::Vector2UInt(2, 0));
	EXPECT_EQ(callCount, 2);
}

TEST(SizeHintTest, WidgetSizeSettersForwardToSizeHint)
{
	spk::Widget widget("Widget");

	widget.setMinimalSize({15, 25});
	widget.setFixedSize({35, 45});
	widget.setMaximalSize({55, 65});

	EXPECT_EQ(widget.minimalSize(), spk::Vector2UInt(15, 25));
	EXPECT_EQ(widget.fixedSize(), spk::Vector2UInt(35, 45));
	EXPECT_EQ(widget.maximalSize(), spk::Vector2UInt(55, 65));
}

TEST(SizeHintTest, TextLabelMinimalSizeTracksTextChanges)
{
	spk::TextLabel label("Label", "Hi");

	const spk::Vector2UInt shortSize = label.minimalSize();
	EXPECT_GT(shortSize.x, 0u);

	label.setText("A considerably longer text");

	const spk::Vector2UInt longSize = label.minimalSize();
	EXPECT_GT(longSize.x, shortSize.x);
}

TEST(SizeHintTest, ChildEditionReleasesParentCachedHints)
{
	spk::Widget parent("Parent");
	spk::TextLabel child("Child", "Hi", &parent);

	int callCount = 0;
	parent.sizeHint().configureMinimalGenerator([&callCount, &child]() {
		++callCount;
		return child.minimalSize();
	});

	const spk::Vector2UInt firstSize = parent.sizeHint().minimal();
	EXPECT_EQ(callCount, 1);

	child.setText("A considerably longer text");

	const spk::Vector2UInt secondSize = parent.sizeHint().minimal();
	EXPECT_EQ(callCount, 2);
	EXPECT_GT(secondSize.x, firstSize.x);
}
