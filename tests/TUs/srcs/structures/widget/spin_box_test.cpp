#include <gtest/gtest.h>

#include "structures/widget/spk_spin_box.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	void clickAt(spk::Widget& p_root, const spk::Vector2Int& p_position)
	{
		spk::MouseModule mouseModule;
		mouseModule.bind(&p_root);
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = p_position})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
		mouseModule.processEvents();
	}
}

TEST(SpinBoxTest, DefaultStateIsZero)
{
	spk::SpinBox<int> spinBox("SpinBox");

	EXPECT_EQ(spinBox.value(), 0);
	EXPECT_EQ(spinBox.step(), 1);
	EXPECT_TRUE(spinBox.isActivated());
	EXPECT_FALSE(spinBox.minimalLimit().has_value());
	EXPECT_FALSE(spinBox.maximalLimit().has_value());
}

TEST(SpinBoxTest, ValueEditReflectsValue)
{
	spk::SpinBox<int> spinBox("SpinBox");

	spinBox.setValue(42);

	EXPECT_EQ(spinBox.value(), 42);
	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("42"));
}

TEST(SpinBoxTest, UpButtonClickIncreasesValueByStep)
{
	spk::SpinBox<int> spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 90, 30));
	spinBox.setStep(5);

	clickAt(spinBox, {75, 15});

	EXPECT_EQ(spinBox.value(), 5);
	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("5"));
}

TEST(SpinBoxTest, DownButtonClickDecreasesValueByStep)
{
	spk::SpinBox<int> spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 90, 30));

	clickAt(spinBox, {15, 15});

	EXPECT_EQ(spinBox.value(), -1);
}

TEST(SpinBoxTest, LimitsClampValue)
{
	spk::SpinBox<int> spinBox("SpinBox");
	spinBox.setLimits(0, 10);

	spinBox.setValue(50);
	EXPECT_EQ(spinBox.value(), 10);

	spinBox.setValue(-5);
	EXPECT_EQ(spinBox.value(), 0);

	spinBox.removeLimits();
	spinBox.setValue(50);
	EXPECT_EQ(spinBox.value(), 50);
}

TEST(SpinBoxTest, SettingLimitClampsCurrentValue)
{
	spk::SpinBox<int> spinBox("SpinBox");
	spinBox.setValue(20);

	spinBox.setMaximalLimit(10);

	EXPECT_EQ(spinBox.value(), 10);
}

TEST(SpinBoxTest, SubscribeToEditionNotifiesValueChanges)
{
	spk::SpinBox<int> spinBox("SpinBox");

	int lastValue = -1;
	auto contract = spinBox.subscribeToEdition([&lastValue](const int& p_value) { lastValue = p_value; });

	spinBox.setValue(7);

	EXPECT_EQ(lastValue, 7);
}

TEST(SpinBoxTest, WorksWithFloatingPointTypes)
{
	spk::SpinBox<double> spinBox("SpinBox");
	spinBox.setStep(0.5);

	spinBox.setValue(1.5);

	EXPECT_DOUBLE_EQ(spinBox.value(), 1.5);
}
