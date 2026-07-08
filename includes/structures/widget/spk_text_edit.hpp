#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/texture/spk_font.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"
#include "type/spk_vertical_alignment.hpp"

namespace spk
{
	class TextEdit : public spk::Widget
	{
	public:
		enum class ValidationState
		{
			Valid,
			Undefined,
			Invalid
		};

		using ValidationCallback = std::function<ValidationState(const spk::Font::Text &)>;
		using EditionCallback = spk::ContractProvider<const spk::Font::Text &>::Callback;
		using EditionContract = spk::ContractProvider<const spk::Font::Text &>::Contract;

	private:
		std::shared_ptr<spk::SpriteSheet> _spriteSheet;
		spk::Vector2Int _cornerSize;
		std::shared_ptr<spk::Font> _font;
		spk::Font::Size _textSize;
		spk::Color _glyphColor;
		spk::Color _outlineColor;
		spk::Color _cursorColor = spk::Color(0.0f, 0.0f, 0.0f, 0.6f);
		float _depth = 0.0f;

		spk::Font::Text _text;
		spk::Font::Text _placeholder = spk::Font::textFromUTF8("Enter text here");
		bool _isObscured = false;
		bool _isEditEnabled = true;
		bool _renderCursor = true;
		bool _isHovered = false;

		size_t _cursor = 0;
		mutable size_t _lowerCursor = 0;
		mutable size_t _higherCursor = 0;
		mutable bool _needLowerCursorUpdate = true;
		mutable bool _needHigherCursorUpdate = true;

		spk::WidgetStyle::Contract _styleEditionContract;

		spk::ContractProvider<const spk::Font::Text &> _onEditionProvider;
		ValidationCallback _validationCallback;

		void _bindStyle(const spk::WidgetStyle &p_style);
		void _configureSizeCache();
		void _notifyEdition();
		[[nodiscard]] ValidationState _validate(const spk::Font::Text &p_text) const;

		[[nodiscard]] spk::Rect2D _innerGeometry() const;
		void _computeCursorsValues() const;

		void _moveCursorLeft();
		void _moveCursorRight();
		void _deleteAtCursor();
		void _backspace();
		void _insertGlyph(char32_t p_glyph);

	protected:
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;
		void _onGeometryChange() override;
		void _onUpdate(const spk::UpdateContext &p_tick) override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		void _onTextInputEvent(spk::TextInputEvent &p_event) override;

	public:
		explicit TextEdit(const std::string &p_name, spk::Widget *p_parent = nullptr);
		TextEdit(
			const std::string &p_name,
			const spk::WidgetStyle &p_style,
			spk::Widget *p_parent = nullptr);

		void applyStyle(const spk::WidgetStyle &p_style) override;
		void useDefaultStyle();
		void useStyle(const spk::WidgetStyle &p_style);

		void setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2Int &p_cornerSize);
		void setFont(std::shared_ptr<spk::Font> p_font);
		void setTextSize(const spk::Font::Size &p_textSize);
		void setGlyphColor(const spk::Color &p_color);
		void setOutlineColor(const spk::Color &p_color);
		void setCursorColor(const spk::Color &p_color);
		void setDepth(float p_depth);

		void setText(const spk::Font::Text &p_text);
		void setText(std::string_view p_text);
		void setPlaceholder(const spk::Font::Text &p_placeholder);
		void setPlaceholder(std::string_view p_placeholder);
		void setObscured(bool p_state);

		void enableEdit();
		void disableEdit();

		EditionContract subscribeToEdition(EditionCallback p_callback);
		void setValidationCallback(ValidationCallback p_callback);
		[[nodiscard]] ValidationState validationState() const;

		[[nodiscard]] bool hasText() const;
		[[nodiscard]] bool isObscured() const;
		[[nodiscard]] bool isEditEnabled() const;
		[[nodiscard]] spk::Font::Text renderedText() const;
		[[nodiscard]] const spk::Font::Text &text() const;
		[[nodiscard]] std::string textAsUTF8() const;
		[[nodiscard]] const spk::Font::Text &placeholder() const;
		[[nodiscard]] size_t cursor() const;

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet> &spriteSheet() const;
		[[nodiscard]] const spk::Vector2Int &cornerSize() const;
		[[nodiscard]] const std::shared_ptr<spk::Font> &font() const;
		[[nodiscard]] const spk::Font::Size &textSize() const;
		[[nodiscard]] const spk::Color &glyphColor() const;
		[[nodiscard]] const spk::Color &outlineColor() const;
		[[nodiscard]] float depth() const;
	};
}
