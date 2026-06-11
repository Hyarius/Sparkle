#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "spk_generated_resources.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"

namespace
{
	[[nodiscard]] std::shared_ptr<spk::SpriteSheet> loadNineSlice(const std::string& p_resourceName)
	{
		const auto& bytes = SPARKLE_GET_RESOURCE(p_resourceName);
		return std::make_shared<spk::SpriteSheet>(
			spk::SpriteSheet::fromRawData(
				std::vector<std::uint8_t>(bytes.begin(), bytes.end()),
				spk::Vector2UInt{3, 3}));
	}

	[[nodiscard]] bool sameColor(const spk::Color& p_left, const spk::Color& p_right)
	{
		return p_left.r == p_right.r &&
			p_left.g == p_right.g &&
			p_left.b == p_right.b &&
			p_left.a == p_right.a;
	}
}

TEST(WidgetStyleTest, SettersUpdateValuesAndNotifySubscribers)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	int notificationCount = 0;
	const spk::WidgetStyle* lastStyle = nullptr;
	auto contract = style.subscribeToEdition(
		[&](const spk::WidgetStyle& p_style)
		{
			++notificationCount;
			lastStyle = &p_style;
		});

	const auto spriteSheet = loadNineSlice("resources/textures/default_nine_slice_darker.png");
	const auto font = std::make_shared<spk::Font>(
		spk::Font::fromRawData(SPARKLE_GET_RESOURCE("resources/fonts/arial.ttf")));

	style.setNineSliceSpriteSheet(spriteSheet);
	style.setNineSliceCornerSize({5, 6});
	style.setFont(font);
	style.setTextSize(spk::Font::Size(21, 2));
	style.setGlyphColor(spk::Color(0.2f, 0.3f, 0.4f, 1.0f));
	style.setOutlineColor(spk::Color(0.7f, 0.6f, 0.5f, 1.0f));
	style.setTextPadding({3, 4});

	EXPECT_EQ(notificationCount, 7);
	EXPECT_EQ(lastStyle, &style);
	EXPECT_EQ(style.nineSliceSpriteSheet(), spriteSheet);
	EXPECT_EQ(style.nineSliceCornerSize(), spk::Vector2Int(5, 6));
	EXPECT_EQ(style.font(), font);
	EXPECT_EQ(style.textSize(), spk::Font::Size(21, 2));
	EXPECT_TRUE(sameColor(style.glyphColor(), spk::Color(0.2f, 0.3f, 0.4f, 1.0f)));
	EXPECT_TRUE(sameColor(style.outlineColor(), spk::Color(0.7f, 0.6f, 0.5f, 1.0f)));
	EXPECT_EQ(style.textPadding(), spk::Vector2Int(3, 4));
}

TEST(WidgetStyleTest, DefaultStyleUsesFixedCornerSizeAndDerivedTextPadding)
{
	const spk::WidgetStyle style = spk::WidgetStyle::makeDefault();

	EXPECT_EQ(style.nineSliceCornerSize(), spk::Vector2Int(8, 8));
	EXPECT_EQ(style.textPadding(), spk::Vector2Int(16, 16));
}

TEST(WidgetStyleTest, InterfaceWindowTitleStyleUsesCompactText)
{
	const spk::WidgetStyle style = spk::WidgetStyle::makeDefaultInterfaceWindowTitle();

	EXPECT_EQ(style.textSize(), spk::Font::Size(12, 0));
	EXPECT_EQ(style.textPadding(), spk::Vector2Int(3, 0));
}

TEST(WidgetStyleTest, SettersSkipNotificationWhenValueIsUnchanged)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	int notificationCount = 0;
	auto contract = style.subscribeToEdition([&](const spk::WidgetStyle&) { ++notificationCount; });

	style.setNineSliceCornerSize(style.nineSliceCornerSize());
	style.setTextSize(style.textSize());
	style.setGlyphColor(style.glyphColor());
	style.setOutlineColor(style.outlineColor());
	style.setTextPadding(style.textPadding());

	EXPECT_EQ(notificationCount, 0);
}

TEST(WidgetStyleTest, SettingNineSliceSpriteSheetPreservesConfiguredCornerSize)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setNineSliceCornerSize({5, 6});

	style.setNineSliceSpriteSheet(loadNineSlice("resources/textures/default_nine_slice_darker.png"));

	EXPECT_EQ(style.nineSliceCornerSize(), spk::Vector2Int(5, 6));
}

TEST(WidgetStyleTest, RejectsInvalidStyleValues)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();

	EXPECT_THROW(style.setNineSliceSpriteSheet(nullptr), std::invalid_argument);
	EXPECT_THROW(style.setFont(nullptr), std::invalid_argument);
	EXPECT_THROW(style.setNineSliceCornerSize({-1, 2}), std::invalid_argument);
	EXPECT_THROW(style.setNineSliceCornerSize({2, -1}), std::invalid_argument);
}

TEST(WidgetStyleTest, CollectionCreatesDefaultBasedNamedStyles)
{
	spk::WidgetStyle& custom = spk::WidgetStyle::Collection::style("unit.custom");

	EXPECT_TRUE(spk::WidgetStyle::Collection::contains("unit.custom"));
	EXPECT_NE(custom.nineSliceSpriteSheet(), nullptr);
	EXPECT_NE(custom.font(), nullptr);
}

TEST(WidgetStyleTest, CollectionSetStyleCopiesValuesAndNotifiesExistingStyle)
{
	spk::WidgetStyle& stored = spk::WidgetStyle::Collection::style("unit.replace");
	int notificationCount = 0;
	auto contract = stored.subscribeToEdition([&](const spk::WidgetStyle&) { ++notificationCount; });

	spk::WidgetStyle replacement = spk::WidgetStyle::makeDefault();
	replacement.setTextSize(spk::Font::Size(31, 3));
	spk::WidgetStyle::Collection::setStyle("unit.replace", replacement);

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(stored.textSize(), spk::Font::Size(31, 3));
}

TEST(WidgetStyleTest, PanelSubscribedToCollectionStyleUpdatesWhenStyleChanges)
{
	spk::WidgetStyle& style = spk::WidgetStyle::Collection::style("unit.panel");
	style.setNineSliceCornerSize({6, 7});

	spk::Panel panel("Panel", style);
	ASSERT_EQ(panel.cornerSize(), spk::Vector2Int(6, 7));

	style.setNineSliceCornerSize({8, 9});

	EXPECT_EQ(panel.cornerSize(), spk::Vector2Int(8, 9));
}

TEST(WidgetStyleTest, TextLabelSubscribedToCollectionStyleUpdatesWhenStyleChanges)
{
	spk::WidgetStyle& style = spk::WidgetStyle::Collection::style("unit.label");
	style.setTextSize(spk::Font::Size(15, 1));
	style.setGlyphColor(spk::Color(0.1f, 0.2f, 0.3f, 1.0f));
	style.setOutlineColor(spk::Color(0.4f, 0.5f, 0.6f, 1.0f));
	style.setTextPadding({2, 3});

	spk::TextLabel label("Label", "Hello", style);
	ASSERT_EQ(label.textSize(), spk::Font::Size(15, 1));

	style.setTextSize(spk::Font::Size(17, 2));
	style.setTextPadding({4, 5});

	EXPECT_EQ(label.textSize(), spk::Font::Size(17, 2));
	EXPECT_EQ(label.padding(), spk::Vector2Int(4, 5));
	EXPECT_TRUE(sameColor(label.glyphColor(), spk::Color(0.1f, 0.2f, 0.3f, 1.0f)));
	EXPECT_TRUE(sameColor(label.outlineColor(), spk::Color(0.4f, 0.5f, 0.6f, 1.0f)));
}

TEST(WidgetStyleTest, ExplicitStyleIsAppliedOnConstruction)
{
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setNineSliceCornerSize({4, 5});
	style.setTextSize(spk::Font::Size(24, 2));

	spk::Panel panel("Panel", style);
	spk::TextLabel label("Label", "Hello", style);

	EXPECT_EQ(panel.cornerSize(), spk::Vector2Int(4, 5));
	EXPECT_EQ(label.textSize(), spk::Font::Size(24, 2));
}

TEST(WidgetStyleTest, DefaultStyleRefreshesWidgetsWhenReplaced)
{
	spk::WidgetStyle originalStyle = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default);
	spk::WidgetStyle firstStyle = originalStyle;
	spk::WidgetStyle secondStyle = originalStyle;

	firstStyle.setTextSize(spk::Font::Size(18, 1));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, firstStyle);
	spk::TextLabel label("Label", "Hello");
	ASSERT_EQ(label.textSize(), spk::Font::Size(18, 1));

	secondStyle.setTextSize(spk::Font::Size(30, 3));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, secondStyle);

	EXPECT_EQ(label.textSize(), spk::Font::Size(30, 3));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, originalStyle);
}

TEST(WidgetStyleTest, DefaultStyleRefreshesWidgetsWhenEdited)
{
	spk::WidgetStyle originalStyle = spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default);
	spk::TextLabel label("Label", "Hello");

	spk::WidgetStyle::Collection::style(spk::WidgetStyle::Collection::Default).setTextSize(spk::Font::Size(28, 2));

	EXPECT_EQ(label.textSize(), spk::Font::Size(28, 2));
	spk::WidgetStyle::Collection::setStyle(spk::WidgetStyle::Collection::Default, originalStyle);
}

TEST(WidgetStyleTest, PushButtonTracksReleasedAndPressedStylesIndependently)
{
	spk::WidgetStyle released = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressed = spk::WidgetStyle::makeDefaultPressed();
	released.setTextSize(spk::Font::Size(12, 0));
	pressed.setTextSize(spk::Font::Size(18, 2));
	released.setTextPadding({1, 2});
	pressed.setTextPadding({3, 4});

	spk::PushButton button("Button", "OK", released, pressed);

	EXPECT_EQ(button.releasedLabel().textSize(), spk::Font::Size(12, 0));
	EXPECT_EQ(button.releasedLabel().padding(), spk::Vector2Int(1, 2));
	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(18, 2));
	EXPECT_EQ(button.pressedLabel().padding(), spk::Vector2Int(3, 4));

	pressed.setTextSize(spk::Font::Size(20, 1));

	EXPECT_EQ(button.pressedLabel().textSize(), spk::Font::Size(20, 1));
}
