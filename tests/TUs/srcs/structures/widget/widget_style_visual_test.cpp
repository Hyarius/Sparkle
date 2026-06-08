#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <functional>

#include <GL/glew.h>

#include "structures/graphics/opengl/opengl_wrapper_test_utils.hpp"
#include "structures/graphics/rendering/command/spk_nine_slice_render_command.hpp"
#include "structures/graphics/rendering/command/spk_text_render_command.hpp"
#include "structures/graphics/rendering/command/spk_viewport_render_command.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"
#include "structures/graphics/rendering/unit/spk_render_unit_builder.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_push_button.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "structures/widget/spk_widget_visual_test_helpers.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path expectedPath(const std::string& p_variant)
	{
		const std::filesystem::path expectedDir = spk::test::expectedDirectory() / "WidgetStyleVisual";
		std::filesystem::create_directories(expectedDir);
		return expectedDir / (p_variant + ".png");
	}

	void renderReferenceImage(
		const std::string& p_variant,
		const spk::Rect2D& p_captureRect,
		const std::function<void(spk::RenderUnitBuilder&)>& p_appendCommands)
	{
		sparkle_test::OpenGLTestContext context(p_captureRect);
		spk::RenderContext& renderContext = context.renderContext();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		spk::RenderUnitBuilder builder;
		spk::Viewport viewport(p_captureRect.atOrigin());
		builder.emplace<spk::ViewportCommand>(viewport);
		p_appendCommands(builder);
		builder.build().execute(renderContext);
		context.gpuRuntime().waitUntilWorkDone();
		context.gpuRuntime().saveScreenshot(expectedPath(p_variant), p_captureRect.atOrigin());
	}

	[[nodiscard]] spk::Vector2Int clampedCornerSize(
		const spk::Vector2Int& p_cornerSize,
		const spk::Rect2D& p_rect)
	{
		return {
			std::min(p_cornerSize.x, static_cast<int>(p_rect.width() / 2)),
			std::min(p_cornerSize.y, static_cast<int>(p_rect.height() / 2))
		};
	}

	[[nodiscard]] spk::Vector2Int textAnchor(
		const spk::Rect2D& p_rect,
		const spk::Vector2Int& p_padding,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		int x = p_rect.left() + p_padding.x;
		if (p_horizontalAlignment == spk::HorizontalAlignment::Centered)
		{
			x = p_rect.left() + static_cast<int>(p_rect.width() / 2);
		}
		else if (p_horizontalAlignment == spk::HorizontalAlignment::Right)
		{
			x = p_rect.right() - p_padding.x;
		}

		int y = p_rect.top() + p_padding.y;
		if (p_verticalAlignment == spk::VerticalAlignment::Centered)
		{
			y = p_rect.top() + static_cast<int>(p_rect.height() / 2);
		}
		else if (p_verticalAlignment == spk::VerticalAlignment::Down)
		{
			y = p_rect.bottom() - p_padding.y;
		}

		return {x, y};
	}

	[[nodiscard]] spk::Vector2Int centeredTextAnchor(const spk::Rect2D& p_rect, const spk::Vector2Int& p_padding)
	{
		return {
			p_rect.left() + static_cast<int>(p_rect.shrink(p_padding).width() / 2) + p_padding.x,
			p_rect.top() + static_cast<int>(p_rect.shrink(p_padding).height() / 2) + p_padding.y
		};
	}

	void appendPanelReference(
		spk::RenderUnitBuilder& p_builder,
		const spk::WidgetStyle& p_style,
		const spk::Rect2D& p_rect)
	{
		p_builder.emplace<spk::NineSliceRenderCommand>(
			*p_style.nineSliceSpriteSheet(),
			p_rect,
			clampedCornerSize(p_style.nineSliceCornerSize(), p_rect));
	}

	void appendTextReference(
		spk::RenderUnitBuilder& p_builder,
		const spk::WidgetStyle& p_style,
		std::string_view p_text,
		const spk::Vector2Int& p_anchor,
		spk::HorizontalAlignment p_horizontalAlignment,
		spk::VerticalAlignment p_verticalAlignment)
	{
		p_builder.emplace<spk::TextRenderCommand>(
			*p_style.font(),
			spk::Font::textFromUTF8(p_text),
			p_style.textSize(),
			p_style.glyphColor(),
			p_style.outlineColor(),
			0.0f,
			p_anchor,
			p_horizontalAlignment,
			p_verticalAlignment);
	}

	void appendPushButtonReference(
		spk::RenderUnitBuilder& p_builder,
		const spk::WidgetStyle& p_style,
		std::string_view p_text,
		const spk::Rect2D& p_rect)
	{
		appendPanelReference(p_builder, p_style, p_rect);
		if (p_text.empty() == false)
		{
			appendTextReference(
				p_builder,
				p_style,
				p_text,
				centeredTextAnchor(p_rect, p_style.textPadding()),
				spk::HorizontalAlignment::Centered,
				spk::VerticalAlignment::Centered);
		}
	}
}

TEST(WidgetStyleVisualTest, PanelRendersNineSliceFromStyle)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);
	const std::string variant = "panel_default_style";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendPanelReference(p_builder, style, captureRect);
	});

	spk::Panel panel("Panel", style);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PanelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 72);
	const std::string variant = "panel_edited_style";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::Panel panel("Panel", style);

	style.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefaultPressed().nineSliceSpriteSheet());
	style.setNineSliceCornerSize({10, 14});

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendPanelReference(p_builder, style, captureRect);
	});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(panel, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, TextLabelRendersTextStylingFromStyle)
{
	const spk::Rect2D captureRect(0, 0, 96, 48);
	const std::string variant = "text_label_style";
	const std::string text = "Hi";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	style.setTextSize(spk::Font::Size(18, 3));
	style.setGlyphColor(spk::Color(0.0f, 0.85f, 0.2f, 1.0f));
	style.setOutlineColor(spk::Color(0.95f, 0.1f, 0.1f, 1.0f));
	style.setTextPadding({6, 5});

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendTextReference(
			p_builder,
			style,
			text,
			textAnchor(captureRect, style.textPadding(), spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top),
			spk::HorizontalAlignment::Left,
			spk::VerticalAlignment::Top);
	});

	spk::TextLabel label("Label", text, style);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, TextLabelVisualRefreshesWhenSubscribedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 96, 48);
	const std::string variant = "text_label_edited_style";
	const std::string text = "Hi";
	spk::WidgetStyle style = spk::WidgetStyle::makeDefault();
	spk::TextLabel label("Label", text, style);

	style.setTextSize(spk::Font::Size(20, 1));
	style.setGlyphColor(spk::Color(0.1f, 0.3f, 1.0f, 1.0f));
	style.setOutlineColor(spk::Color(1.0f, 0.9f, 0.0f, 1.0f));
	style.setTextPadding({12, 8});

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendTextReference(
			p_builder,
			style,
			text,
			textAnchor(captureRect, style.textPadding(), spk::HorizontalAlignment::Left, spk::VerticalAlignment::Top),
			spk::HorizontalAlignment::Left,
			spk::VerticalAlignment::Top);
	});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(label, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonRendersReleasedStyleWhenIdle)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_released_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	releasedStyle.setTextSize(spk::Font::Size(16, 2));
	releasedStyle.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
	releasedStyle.setTextPadding({8, 4});
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendPushButtonReference(p_builder, releasedStyle, text, captureRect);
	});

	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonRendersPressedStyleWhilePressed)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_pressed_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	pressedStyle.setTextSize(spk::Font::Size(18, 2));
	pressedStyle.setGlyphColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	pressedStyle.setTextPadding({10, 6});

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendPushButtonReference(p_builder, pressedStyle, text, captureRect);
	});

	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	button.setGeometry(captureRect);

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}

TEST(WidgetStyleVisualTest, PushButtonPressedVisualRefreshesWhenPressedStyleChanges)
{
	const spk::Rect2D captureRect(0, 0, 112, 56);
	const std::string variant = "push_button_pressed_edited_style";
	const std::string text = "OK";
	spk::WidgetStyle releasedStyle = spk::WidgetStyle::makeDefault();
	spk::WidgetStyle pressedStyle = spk::WidgetStyle::makeDefaultPressed();
	spk::PushButton button("Button", text, releasedStyle, pressedStyle);
	button.setGeometry(captureRect);

	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::MouseEventRecord pressEvent = spk::MouseEventRecord(spk::makeEventRecord(spk::MouseButtonPressedRecord{
		.button = spk::Mouse::Left}));
	button.dispatchMouseEvent(pressEvent, mouse);

	pressedStyle.setNineSliceSpriteSheet(spk::WidgetStyle::makeDefault().nineSliceSpriteSheet());
	pressedStyle.setNineSliceCornerSize({9, 11});
	pressedStyle.setTextSize(spk::Font::Size(14, 4));
	pressedStyle.setGlyphColor(spk::Color(0.0f, 0.0f, 0.0f, 1.0f));
	pressedStyle.setOutlineColor(spk::Color(1.0f, 1.0f, 1.0f, 1.0f));
	pressedStyle.setTextPadding({4, 2});

	renderReferenceImage(variant, captureRect, [&](spk::RenderUnitBuilder& p_builder)
	{
		appendPushButtonReference(p_builder, pressedStyle, text, captureRect);
	});

	const sparkle_test::ImageComparisonResult result =
		spk::test::compareSnapshot(button, "WidgetStyleVisual", variant, captureRect);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0u);
}
