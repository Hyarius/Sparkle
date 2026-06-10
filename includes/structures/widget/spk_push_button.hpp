#pragma once

#include <functional>
#include <string>
#include <string_view>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_image_label.hpp"
#include "structures/widget/spk_panel.hpp"
#include "structures/widget/spk_text_label.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_horizontal_alignment.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class PushButton : public spk::Widget
	{
	public:
		using Callback = std::function<void()>;
		using Contract = spk::ContractProvider<>::Contract;

	private:
		bool _isHovered = false;
		bool _isPressed = false;
		spk::ContractProvider<> _clickProvider;

		spk::Panel _releasedBackground;
		spk::Panel _pressedBackground;
		spk::TextLabel _releasedLabel;
		spk::TextLabel _pressedLabel;
		spk::ImageLabel _releasedIcon;
		spk::ImageLabel _pressedIcon;
		bool _hasIcon = false;
		bool _isFlat = false;

		void _refreshState();
		void _refreshIconGeometry();

	protected:
		void _onGeometryChange() override;
		void _onMouseMovedEvent(spk::MouseMovedEvent& p_event) override;
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

		void applyStyle(const spk::WidgetStyle& p_style) override;
		void applyStyle(const spk::WidgetStyle& p_releasedStyle, const spk::WidgetStyle& p_pressedStyle);
		Contract subscribeToClick(Callback p_callback);

		void useDefaultStyles();
		void setText(const spk::Font::Text& p_text);
		void setText(std::string_view p_text);
		void setAlignment(spk::HorizontalAlignment p_horizontal, spk::VerticalAlignment p_vertical);
		void setIcon(std::shared_ptr<spk::Texture> p_texture, const spk::Texture::Section& p_section = {{0.0f, 0.0f}, {1.0f, 1.0f}});
		void setIcon(std::shared_ptr<spk::SpriteSheet> p_spriteSheet, size_t p_spriteID);
		void removeIcon();
		void setFlat(bool p_state);

		[[nodiscard]] bool hasIcon() const;
		[[nodiscard]] bool isFlat() const;

		[[nodiscard]] bool isHovered() const;
		[[nodiscard]] bool isPressed() const;

		[[nodiscard]] spk::Panel& releasedBackground();
		[[nodiscard]] const spk::Panel& releasedBackground() const;
		[[nodiscard]] spk::Panel& pressedBackground();
		[[nodiscard]] const spk::Panel& pressedBackground() const;
		[[nodiscard]] spk::TextLabel& releasedLabel();
		[[nodiscard]] const spk::TextLabel& releasedLabel() const;
		[[nodiscard]] spk::TextLabel& pressedLabel();
		[[nodiscard]] const spk::TextLabel& pressedLabel() const;
		[[nodiscard]] spk::ImageLabel& releasedIcon();
		[[nodiscard]] const spk::ImageLabel& releasedIcon() const;
		[[nodiscard]] spk::ImageLabel& pressedIcon();
		[[nodiscard]] const spk::ImageLabel& pressedIcon() const;
	};
}
