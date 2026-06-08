#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class PushButton : public spk::Panel
	{
	public:
		using Callback = std::function<void()>;
		using Contract = spk::ContractProvider<>::Contract;

	private:
		struct VisualStyle
		{
			std::shared_ptr<spk::SpriteSheet> spriteSheet;
			spk::Vector2Int cornerSize = {0, 0};
			std::shared_ptr<spk::Font> font;
			spk::Font::Size textSize;
			spk::Color glyphColor;
			spk::Color outlineColor;
			spk::Vector2Int textPadding = {0, 0};
		};

		VisualStyle _releasedStyle;
		VisualStyle _pressedStyle;
		spk::WidgetStyle::Contract _releasedStyleEditionContract;
		spk::WidgetStyle::Contract _pressedStyleEditionContract;
		std::shared_ptr<spk::Font> _font;
		spk::Font::Text _text;
		spk::Font::Size _textSize;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		spk::Vector2Int _textPadding = {0, 0};
		float _textDepth = 0.0f;
		bool _isHovered = false;
		bool _isPressed = false;
		spk::ContractProvider<> _clickProvider;

		[[nodiscard]] bool _containsMouse(const spk::Mouse& p_mouse) const;
		static VisualStyle _visualStyleFrom(const spk::WidgetStyle& p_style);
		void _bindReleasedStyle(const spk::WidgetStyle& p_style);
		void _bindPressedStyle(const spk::WidgetStyle& p_style);
		void _applyVisualStyle(const VisualStyle& p_style);
		void _refreshVisualStyle();

	protected:
		void _applyStyle(const spk::WidgetStyle& p_style) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;
		void _onMouseMovedEvent(spk::MouseMovedEvent& p_event) override;
		void _onMouseLeftEvent(spk::MouseLeftWindowEvent& p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event) override;

	public:
		explicit PushButton(const std::string& p_name, spk::Widget* p_parent = nullptr);
		PushButton(
			const std::string& p_name,
			std::string_view p_text,
			spk::Widget* p_parent = nullptr);
		PushButton(
			const std::string& p_name,
			std::string_view p_text,
			const spk::WidgetStyle& p_style,
			spk::Widget* p_parent = nullptr);
		PushButton(
			const std::string& p_name,
			std::string_view p_text,
			const spk::WidgetStyle& p_releasedStyle,
			const spk::WidgetStyle& p_pressedStyle,
			spk::Widget* p_parent = nullptr);

		Contract subscribeToClick(Callback p_callback);

		void useDefaultStyles();
		void useStyles(const spk::WidgetStyle& p_releasedStyle, const spk::WidgetStyle& p_pressedStyle);
		void setReleasedSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setPressedSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setText(const spk::Font::Text& p_text);
		void setText(std::string_view p_text);
		void setTextSize(const spk::Font::Size& p_textSize);
		void setGlyphColor(const spk::Color& p_color);
		void setOutlineColor(const spk::Color& p_color);
		void setTextPadding(const spk::Vector2Int& p_padding);
		void setTextDepth(float p_depth);

		[[nodiscard]] bool isHovered() const;
		[[nodiscard]] bool isPressed() const;
		[[nodiscard]] const spk::Font::Text& text() const;
		[[nodiscard]] const spk::Font::Size& textSize() const;
		[[nodiscard]] const spk::Color& glyphColor() const;
		[[nodiscard]] const spk::Color& outlineColor() const;
		[[nodiscard]] const spk::Vector2Int& textPadding() const;
	};
}
