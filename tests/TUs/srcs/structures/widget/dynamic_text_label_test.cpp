#include <gtest/gtest.h>

#include <string>

#include "structures/widget/spk_dynamic_text_label.hpp"

namespace
{
	[[nodiscard]] spk::UpdateTick makeTick()
	{
		return spk::UpdateTick{};
	}
}

TEST(DynamicTextLabelTest, SetTextProducerRefreshesImmediately)
{
	spk::DynamicTextLabel label("Dynamic");

	label.setTextProducer([]() { return std::string("produced"); });

	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("produced"));
}

TEST(DynamicTextLabelTest, ProducerConstructorRefreshesImmediately)
{
	spk::DynamicTextLabel label("Dynamic", []() { return std::string("initial"); });

	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("initial"));
}

TEST(DynamicTextLabelTest, UpdateWithoutProducerKeepsText)
{
	spk::DynamicTextLabel label("Dynamic");
	spk::UpdateTick tick = makeTick();

	label.update(tick);

	EXPECT_TRUE(label.text().empty());
}

TEST(DynamicTextLabelTest, UpdateRefreshesAfterTimeout)
{
	int callCount = 0;
	spk::DynamicTextLabel label("Dynamic");
	label.setTextProducer([&callCount]()
	{
		++callCount;
		return std::to_string(callCount);
	});
	label.setRefreshDuration(spk::Duration(0.0L, spk::TimeUnit::Millisecond));

	spk::UpdateTick tick = makeTick();
	label.update(tick);
	label.update(tick);

	EXPECT_GE(callCount, 3);
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8(std::to_string(callCount)));
}

TEST(DynamicTextLabelTest, RefreshForcesImmediateUpdate)
{
	int callCount = 0;
	spk::DynamicTextLabel label("Dynamic");
	label.setTextProducer([&callCount]()
	{
		++callCount;
		return std::to_string(callCount);
	});

	ASSERT_EQ(callCount, 1);

	label.refresh();

	EXPECT_EQ(callCount, 2);
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("2"));
}

TEST(DynamicTextLabelTest, UpdateDoesNotRefreshBeforeTimeout)
{
	int callCount = 0;
	spk::DynamicTextLabel label("Dynamic");
	label.setTextProducer([&callCount]()
	{
		++callCount;
		return std::to_string(callCount);
	});

	spk::UpdateTick tick = makeTick();
	label.update(tick);
	label.update(tick);

	EXPECT_EQ(callCount, 1);
}
