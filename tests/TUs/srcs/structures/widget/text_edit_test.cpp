#include <gtest/gtest.h>

#include <stdexcept>

#include "structures/widget/spk_text_edit.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"
#include "structures/system/device/window/window_test_utils.hpp"

namespace
{
	class TextEditTester : public spk::TextEdit
	{
	public:
		using spk::TextEdit::TextEdit;
		using spk::TextEdit::_onUpdate;
	};

	void clickInside(spk::TextEdit& p_textEdit)
	{
		spk::MouseModule mouseModule;
		mouseModule.bind(&p_textEdit);
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseMovedRecord{
			.position = {
				p_textEdit.geometry().x() + 5,
				p_textEdit.geometry().y() + 5}})));
		mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
		mouseModule.processEvents();
	}

	void typeGlyph(spk::TextEdit& p_textEdit, char32_t p_glyph)
	{
		sparkle_test::sendKeyboardEvent(
			p_textEdit,
			spk::KeyboardEventRecord(spk::makeEventRecord(spk::TextInputRecord{.glyph = p_glyph})));
	}

	void pressKey(spk::TextEdit& p_textEdit, spk::Keyboard::Key p_key)
	{
		sparkle_test::sendKeyboardEvent(
			p_textEdit,
			spk::KeyboardEventRecord(spk::makeEventRecord(spk::KeyPressedRecord{.key = p_key})));
	}
}

TEST(TextEditTest, DefaultStateUsesPlaceholder)
{
	spk::TextEdit textEdit("TextEdit");

	EXPECT_TRUE(textEdit.isActivated());
	EXPECT_TRUE(textEdit.isEditEnabled());
	EXPECT_FALSE(textEdit.hasText());
	EXPECT_FALSE(textEdit.isObscured());
	EXPECT_EQ(textEdit.renderedText(), spk::Font::textFromUTF8("Enter text here"));
}

TEST(TextEditTest, BuildsBackgroundAndTextRenderCommands)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	auto renderUnit = textEdit.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 2u);
}

TEST(TextEditTest, RendersCursorWhenFocused)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	clickInside(textEdit);

	ASSERT_TRUE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	auto renderUnit = textEdit.renderUnit();

	ASSERT_NE(renderUnit, nullptr);
	EXPECT_EQ(renderUnit->size(), 3u);

	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, SetTextMovesCursorToEnd)
{
	spk::TextEdit textEdit("TextEdit");

	textEdit.setText("Hello");

	EXPECT_TRUE(textEdit.hasText());
	EXPECT_EQ(textEdit.text(), spk::Font::textFromUTF8("Hello"));
	EXPECT_EQ(textEdit.textAsUTF8(), "Hello");
	EXPECT_EQ(textEdit.cursor(), 5u);
}

TEST(TextEditTest, ObscuredTextRendersAsterisks)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setText("secret");

	textEdit.setObscured(true);

	EXPECT_TRUE(textEdit.isObscured());
	EXPECT_EQ(textEdit.renderedText(), spk::Font::textFromUTF8("******"));
	EXPECT_EQ(textEdit.text(), spk::Font::textFromUTF8("secret"));
}

TEST(TextEditTest, TypingInsertsGlyphsWhenFocused)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	clickInside(textEdit);
	ASSERT_TRUE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	typeGlyph(textEdit, U'H');
	typeGlyph(textEdit, U'i');

	EXPECT_EQ(textEdit.text(), spk::Font::textFromUTF8("Hi"));
	EXPECT_EQ(textEdit.cursor(), 2u);

	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, TypingWithoutFocusIsIgnored)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	typeGlyph(textEdit, U'X');

	EXPECT_FALSE(textEdit.hasText());
}

TEST(TextEditTest, ControlGlyphsAreIgnored)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	clickInside(textEdit);
	typeGlyph(textEdit, U'\t');
	typeGlyph(textEdit, static_cast<char32_t>(8));

	EXPECT_FALSE(textEdit.hasText());

	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, BackspaceRemovesGlyphBeforeCursor)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));
	textEdit.setText("ab");

	clickInside(textEdit);
	pressKey(textEdit, spk::Keyboard::Backspace);

	EXPECT_EQ(textEdit.text(), spk::Font::textFromUTF8("a"));
	EXPECT_EQ(textEdit.cursor(), 1u);

	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, ArrowKeysAndDeleteEditAtCursor)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));
	textEdit.setText("abc");

	clickInside(textEdit);
	pressKey(textEdit, spk::Keyboard::LeftArrow);
	pressKey(textEdit, spk::Keyboard::LeftArrow);
	pressKey(textEdit, spk::Keyboard::Delete);

	EXPECT_EQ(textEdit.text(), spk::Font::textFromUTF8("ac"));
	EXPECT_EQ(textEdit.cursor(), 1u);

	pressKey(textEdit, spk::Keyboard::RightArrow);
	EXPECT_EQ(textEdit.cursor(), 2u);

	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, EscapeReleasesKeyboardFocus)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	clickInside(textEdit);
	ASSERT_TRUE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	pressKey(textEdit, spk::Keyboard::Escape);

	EXPECT_FALSE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));
}

TEST(TextEditTest, DisabledEditIgnoresMouseAndKeyboard)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));

	textEdit.disableEdit();
	EXPECT_FALSE(textEdit.isEditEnabled());

	clickInside(textEdit);
	EXPECT_FALSE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	typeGlyph(textEdit, U'X');
	EXPECT_FALSE(textEdit.hasText());

	textEdit.enableEdit();
	EXPECT_TRUE(textEdit.isEditEnabled());
}

TEST(TextEditTest, SetPlaceholderUpdatesRenderedText)
{
	spk::TextEdit textEdit("TextEdit");

	textEdit.setPlaceholder("Type here...");

	EXPECT_EQ(textEdit.placeholder(), spk::Font::textFromUTF8("Type here..."));
	EXPECT_EQ(textEdit.renderedText(), spk::Font::textFromUTF8("Type here..."));
}

TEST(TextEditTest, SetSpriteSheetRejectsInvalidValues)
{
	spk::TextEdit textEdit("TextEdit");

	EXPECT_THROW(textEdit.setSpriteSheet(nullptr), std::invalid_argument);
}

TEST(TextEditTest, SetFontRejectsNull)
{
	spk::TextEdit textEdit("TextEdit");

	EXPECT_THROW(textEdit.setFont(nullptr), std::invalid_argument);
}

TEST(TextEditTest, SetCornerSizeRejectsNegativeComponents)
{
	spk::TextEdit textEdit("TextEdit");

	EXPECT_THROW(textEdit.setCornerSize({-1, 0}), std::invalid_argument);
	EXPECT_THROW(textEdit.setCornerSize({0, -1}), std::invalid_argument);
}

TEST(TextEditTest, StyleEditionRefreshesProperties)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextEdit textEdit("TextEdit", style);

	style.setTextSize(spk::Font::Size(24, 2));

	EXPECT_EQ(textEdit.textSize(), spk::Font::Size(24, 2));
}

TEST(TextEditTest, PublicStylePropertiesReflectSetters)
{
	spk::TextEdit textEdit("TextEdit");
	const auto sheet = textEdit.spriteSheet();
	const auto font = textEdit.font();
	ASSERT_NE(sheet, nullptr);
	ASSERT_NE(font, nullptr);

	textEdit.setSpriteSheet(sheet);
	textEdit.setFont(font);
	textEdit.setCornerSize({7, 8});
	textEdit.setTextSize(spk::Font::Size(19, 2));
	textEdit.setGlyphColor(spk::Color(0.1f, 0.2f, 0.3f, 0.4f));
	textEdit.setOutlineColor(spk::Color(0.5f, 0.6f, 0.7f, 0.8f));
	textEdit.setCursorColor(spk::Color(0.9f, 0.1f, 0.2f, 0.3f));
	textEdit.setDepth(4.5f);

	EXPECT_EQ(textEdit.spriteSheet(), sheet);
	EXPECT_EQ(textEdit.font(), font);
	EXPECT_EQ(textEdit.cornerSize(), spk::Vector2Int(7, 8));
	EXPECT_EQ(textEdit.textSize(), spk::Font::Size(19, 2));
	EXPECT_FLOAT_EQ(textEdit.glyphColor().r, 0.1f);
	EXPECT_FLOAT_EQ(textEdit.outlineColor().g, 0.6f);
	EXPECT_FLOAT_EQ(textEdit.depth(), 4.5f);

	textEdit.setCursorColor(spk::Color(0.9f, 0.1f, 0.2f, 0.3f));
	textEdit.setDepth(4.5f);
}

TEST(TextEditTest, RejectsSpriteSheetWithoutThreeByThreeGrid)
{
	spk::TextEdit textEdit("TextEdit");
	auto invalidSheet = std::make_shared<spk::SpriteSheet>();

	EXPECT_THROW(textEdit.setSpriteSheet(invalidSheet), std::invalid_argument);
}

TEST(TextEditTest, TextAsUtf8EncodesEveryUnicodeWidth)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setText(spk::Font::Text{U'A', U'\u00E9', U'\u20AC', U'\U0001F600'});

	const std::string expected{
		'A',
		static_cast<char>(0xC3), static_cast<char>(0xA9),
		static_cast<char>(0xE2), static_cast<char>(0x82), static_cast<char>(0xAC),
		static_cast<char>(0xF0), static_cast<char>(0x9F), static_cast<char>(0x98), static_cast<char>(0x80)};
	EXPECT_EQ(textEdit.textAsUTF8(), expected);
}

TEST(TextEditTest, ValidationStateUsesCurrentTextAndCallback)
{
	spk::TextEdit textEdit("TextEdit");
	EXPECT_EQ(textEdit.validationState(), spk::TextEdit::ValidationState::Valid);

	textEdit.setText("value");
	textEdit.setValidationCallback([](const spk::Font::Text &p_text)
	{
		return p_text == spk::Font::textFromUTF8("value")
			? spk::TextEdit::ValidationState::Undefined
			: spk::TextEdit::ValidationState::Invalid;
	});

	EXPECT_EQ(textEdit.validationState(), spk::TextEdit::ValidationState::Undefined);
}

TEST(TextEditTest, BoundaryAndUnrelatedKeysAreIgnored)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));
	clickInside(textEdit);

	pressKey(textEdit, spk::Keyboard::LeftArrow);
	pressKey(textEdit, spk::Keyboard::RightArrow);
	pressKey(textEdit, spk::Keyboard::Delete);
	pressKey(textEdit, spk::Keyboard::Backspace);
	pressKey(textEdit, spk::Keyboard::A);

	EXPECT_TRUE(textEdit.text().empty());
	EXPECT_EQ(textEdit.cursor(), 0u);
	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, NonLeftClickIsIgnoredAndOutsideLeftClickReleasesFocus)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));
	clickInside(textEdit);
	ASSERT_TRUE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	spk::MouseModule mouseModule;
	mouseModule.bind(&textEdit);
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(
		spk::MouseMovedRecord{.position = {300, 300}})));
	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(
		spk::MouseButtonPressedRecord{.button = spk::Mouse::Right})));
	mouseModule.processEvents();
	EXPECT_TRUE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));

	mouseModule.pushEvent(spk::MouseEventRecord(spk::makeEventRecord(
		spk::MouseButtonPressedRecord{.button = spk::Mouse::Left})));
	mouseModule.processEvents();
	EXPECT_FALSE(textEdit.hasFocus(spk::Widget::FocusType::Keyboard));
}

TEST(TextEditTest, CursorBlinkUpdateRunsWithAndWithoutKeyboardFocus)
{
	TextEditTester textEdit("TextEdit");
	textEdit.setGeometry(spk::Rect2D(0, 0, 200, 40));
	spk::UpdateTick hiddenTick{};
	hiddenTick.timestamp = spk::Timestamp(250.0L, spk::TimeUnit::Millisecond);
	EXPECT_NO_THROW(textEdit._onUpdate(hiddenTick));

	clickInside(textEdit);
	spk::UpdateTick shownTick{};
	shownTick.timestamp = spk::Timestamp(500.0L, spk::TimeUnit::Millisecond);
	EXPECT_NO_THROW(textEdit._onUpdate(shownTick));
	textEdit.releaseFocus(spk::Widget::FocusType::Keyboard);
}

TEST(TextEditTest, EmptyGeometryBuildsEmptyRenderUnit)
{
	spk::TextEdit textEdit("TextEdit");

	const auto unit = textEdit.renderUnit();

	ASSERT_NE(unit, nullptr);
	EXPECT_TRUE(unit->empty());
}

TEST(TextEditTest, PlaceholderContributesToMinimalSize)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setPlaceholder("A reasonably long placeholder");

	EXPECT_GT(textEdit.minimalSize().x, 0u);
	EXPECT_GT(textEdit.minimalSize().y, 0u);
}

TEST(TextEditTest, RepeatedObscuredStateAndPlaceholderWithTextAreSafe)
{
	spk::TextEdit textEdit("TextEdit");
	textEdit.setObscured(false);
	textEdit.setText("content");
	textEdit.setPlaceholder("unused");

	EXPECT_EQ(textEdit.renderedText(), spk::Font::textFromUTF8("content"));
}

TEST(TextEditVisualTest, RendersEmptyWithPlaceholder)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setPlaceholder("Enter text here");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "empty_placeholder", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextEditVisualTest, RendersWithText)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setText("Hello World");

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "with_text", captureRect);

	EXPECT_TRUE(result.matches);
}

TEST(TextEditVisualTest, RendersObscuredText)
{
	const spk::Rect2D captureRect(0, 0, 240, 48);

	spk::TextEdit edit("Edit");
	edit.applyStyle(spk::WidgetStyle::makeDefault());
	edit.setText("password");
	edit.setObscured(true);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(edit, "TextEditVisual", "obscured", captureRect);

	EXPECT_TRUE(result.matches);
}
