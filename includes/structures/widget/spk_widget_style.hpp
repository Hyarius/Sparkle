#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	class WidgetStyle
	{
	public:
		using Contract = spk::ContractProvider<const spk::WidgetStyle*>::Contract;
		using Callback = std::function<void(const spk::WidgetStyle&)>;
		class Collection;

	private:
		std::shared_ptr<spk::SpriteSheet> _nineSliceSpriteSheet;
		spk::Vector2Int _nineSliceCornerSize;

		std::shared_ptr<spk::Font> _font;
		spk::Font::Size _textSize;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		spk::Vector2Int _textPadding;

		mutable spk::ContractProvider<const spk::WidgetStyle*> _editionProvider;

		void _copyValuesFrom(const spk::WidgetStyle& p_other);

	public:
		WidgetStyle();
		WidgetStyle(const WidgetStyle& p_other);
		WidgetStyle& operator=(const WidgetStyle& p_other);
		WidgetStyle(WidgetStyle&& p_other) noexcept;
		WidgetStyle& operator=(WidgetStyle&& p_other) noexcept;

		static WidgetStyle makeDefault();
		static WidgetStyle makeDefaultPressed();

		void notifyEdition() const;
		Contract subscribeToEdition(Callback p_callback) const;

		void setNineSliceSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setNineSliceCornerSize(const spk::Vector2Int& p_cornerSize);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setTextSize(const spk::Font::Size& p_textSize);
		void setGlyphColor(const spk::Color& p_color);
		void setOutlineColor(const spk::Color& p_color);
		void setTextPadding(const spk::Vector2Int& p_padding);

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet>& nineSliceSpriteSheet() const;
		[[nodiscard]] const spk::Vector2Int& nineSliceCornerSize() const;
		[[nodiscard]] const std::shared_ptr<spk::Font>& font() const;
		[[nodiscard]] const spk::Font::Size& textSize() const;
		[[nodiscard]] const spk::Color& glyphColor() const;
		[[nodiscard]] const spk::Color& outlineColor() const;
		[[nodiscard]] const spk::Vector2Int& textPadding() const;
	};

	class WidgetStyle::Collection
	{
	private:
		std::unordered_map<std::string, spk::WidgetStyle> _styles;

		Collection();

	public:
		static constexpr std::string_view Default = "default";
		static constexpr std::string_view DefaultPressed = "default.pressed";

		Collection(const Collection&) = delete;
		Collection& operator=(const Collection&) = delete;

		static Collection& instance();

		static bool contains(std::string_view p_name);
		static spk::WidgetStyle& style(std::string_view p_name);
		static void setStyle(std::string_view p_name, const spk::WidgetStyle& p_style);
	};
}
