#pragma once

#include <memory>
#include <string>

#include "structures/graphics/texture/spk_sprite_sheet.hpp"
#include "structures/widget/spk_push_button.hpp"

namespace spk
{
	class IconButton : public spk::PushButton
	{
	private:
		std::shared_ptr<spk::SpriteSheet> _iconset;
		size_t _iconSpriteID = 0;

		using spk::PushButton::setText;

		void _refreshIcon();

	public:
		explicit IconButton(const std::string& p_name, spk::Widget* p_parent = nullptr);
		IconButton(
			const std::string& p_name,
			std::shared_ptr<spk::SpriteSheet> p_iconset,
			size_t p_iconSpriteID,
			spk::Widget* p_parent = nullptr);

		void setIconset(std::shared_ptr<spk::SpriteSheet> p_iconset);
		void setIconSpriteID(size_t p_spriteID);
		void setIconSpriteID(const spk::Vector2UInt& p_spriteCoordinates);

		[[nodiscard]] const std::shared_ptr<spk::SpriteSheet>& iconset() const;
		[[nodiscard]] size_t iconSpriteID() const;
	};
}
