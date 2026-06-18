#include <gtest/gtest.h>

#include "structures/widget/spk_numeric_spin_box.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	void clickAt(spk::Widget& p_widget, const spk::Vector2Int& p_position)
	{
		spk::MouseModule mouseModule;
		mouseModule.bind(&p_widget);
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{.position = p_position})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonReleasedRecord{.button = spk::Mouse::Left})));
		mouseModule.processEvents();
	}

	void typeGlyph(spk::TextEdit& p_textEdit, char32_t p_glyph)
	{
		sparkle_test::sendKeyboardEvent(p_textEdit, spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{.glyph = p_glyph})));
	}
}

TEST(NumericSpinBoxTest, DefaultStateIsZero)
{
	spk::IntSpinBox spinBox("SpinBox");

	EXPECT_EQ(spinBox.value(), 0);
	EXPECT_EQ(spinBox.step(), 1);
	EXPECT_TRUE(spinBox.isActivated());
	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("0"));
}

TEST(NumericSpinBoxTest, IncreaseAndDecreaseUseStep)
{
	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setStep(5);

	spinBox.increase();
	EXPECT_EQ(spinBox.value(), 5);

	spinBox.decrease();
	spinBox.decrease();
	EXPECT_EQ(spinBox.value(), -5);
	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("-5"));
}

TEST(NumericSpinBoxTest, ButtonsModifyValue)
{
	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 120, 30));

	clickAt(spinBox, {105, 15});
	EXPECT_EQ(spinBox.value(), 1);

	clickAt(spinBox, {75, 15});
	EXPECT_EQ(spinBox.value(), 0);
}

TEST(NumericSpinBoxTest, TypingValidNumberSyncsValue)
{
	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 120, 30));

	int lastValue = -1;
	auto contract = spinBox.subscribeToEdition([&lastValue](const int& p_value) { lastValue = p_value; });

	clickAt(spinBox, {30, 15});
	ASSERT_TRUE(spinBox.valueEdit().hasFocus(spk::Widget::FocusType::Keyboard));

	typeGlyph(spinBox.valueEdit(), U'4');
	typeGlyph(spinBox.valueEdit(), U'2');

	EXPECT_EQ(spinBox.value(), 42);
	EXPECT_EQ(lastValue, 42);

	spinBox.valueEdit().releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(NumericSpinBoxTest, TypingLetterIsRejectedByValidation)
{
	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 120, 30));

	clickAt(spinBox, {30, 15});
	ASSERT_TRUE(spinBox.valueEdit().hasFocus(spk::Widget::FocusType::Keyboard));

	typeGlyph(spinBox.valueEdit(), U'a');

	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("0"));
	EXPECT_EQ(spinBox.value(), 0);

	spinBox.valueEdit().releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(NumericSpinBoxTest, UnsignedRejectsMinusSign)
{
	spk::UnsignedIntSpinBox spinBox("SpinBox");
	spinBox.setGeometry(spk::Rect2D(0, 0, 120, 30));
	spinBox.valueEdit().setText("");

	clickAt(spinBox, {30, 15});
	typeGlyph(spinBox.valueEdit(), U'-');

	EXPECT_TRUE(spinBox.valueEdit().text().empty());

	spinBox.valueEdit().releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(NumericSpinBoxTest, FloatSpinBoxAcceptsDecimalValues)
{
	spk::FloatSpinBox spinBox("SpinBox");
	spinBox.setStep(0.5f);

	spinBox.setValue(1.0f);
	spinBox.increase();

	EXPECT_FLOAT_EQ(spinBox.value(), 1.5f);
}

TEST(NumericSpinBoxTest, SetValueRefreshesText)
{
	spk::IntSpinBox spinBox("SpinBox");

	spinBox.setValue(123);

	EXPECT_EQ(spinBox.valueEdit().text(), spk::Font::textFromUTF8("123"));
}

TEST(NumericSpinBoxVisualTest, RendersIntSpinBoxDefault)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::IntSpinBox spinBox("SpinBox");

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "int_default", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(NumericSpinBoxVisualTest, RendersWithCustomValue)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::IntSpinBox spinBox("SpinBox");
	spinBox.setValue(42);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "int_value_42", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(NumericSpinBoxVisualTest, RendersFloatSpinBox)
{
	const spk::Rect2D captureRect(0, 0, 200, 40);

	spk::FloatSpinBox spinBox("SpinBox");
	spinBox.setValue(3.14f);

	const sparkle_test::ImageComparisonResult result = spk::test::compareSnapshot(spinBox, "NumericSpinBoxVisual", "float_pi", captureRect);

	EXPECT_TRUE(result.matches);
}
