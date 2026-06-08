#pragma once

#include <memory>
#include <string>

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/widget/spk_widget.hpp"
#include "structures/widget/spk_widget_style.hpp"

namespace spk
{
	class Panel : public spk::Widget
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _spriteSheet;
		spk::Vector2Int _cornerSize;
		float _depth = 0.0f;
		spk::WidgetStyle::Contract _styleEditionContract;

		void _bindStyle(const spk::WidgetStyle& p_style);

	protected:
		virtual void _applyStyle(const spk::WidgetStyle& p_style);
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;

	public:
		explicit Panel(const std::string& p_name, spk::Widget* p_parent = nullptr);
		Panel(
			const std::string& p_name,
			const spk::WidgetStyle& p_style,
			spk::Widget* p_parent = nullptr);
		Panel(
			const std::string& p_name,
			std::shared_ptr<spk::SpriteSheet> p_spriteSheet,
			spk::Widget* p_parent = nullptr);

		void useDefaultStyle();
		void useStyle(const spk::WidgetStyle& p_style);
		void setSpriteSheet(std::shared_ptr<spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2Int& p_cornerSize);
		void setDepth(float p_depth);

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet>& spriteSheet() const;
		[[nodiscard]] const spk::Vector2Int& cornerSize() const;
		[[nodiscard]] float depth() const;
	};
}
