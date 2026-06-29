#pragma once

#include "structures/game_engine/spk_component_2d.hpp"
#include "structures/graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	class TextureMesh2D;

	class SpriteRenderer2D : public spk::Component2D
	{
	private:
		const spk::TextureMesh2D *_mesh = nullptr;
		const spk::SpriteSheet *_spriteSheet = nullptr;
		spk::SpriteSheet::Sprite _sprite = spk::SpriteSheet::Sprite::whole;

	public:
		SpriteRenderer2D() = default;

		[[nodiscard]] const spk::SpriteSheet *spriteSheet() const
		{
			return _spriteSheet;
		}

		void setSpriteSheet(const spk::SpriteSheet *p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;
		}

		[[nodiscard]] const spk::TextureMesh2D *mesh() const
		{
			return _mesh;
		}

		void setMesh(const spk::TextureMesh2D *p_mesh)
		{
			_mesh = p_mesh;
		}

		[[nodiscard]] const spk::SpriteSheet::Sprite &sprite() const
		{
			return _sprite;
		}

		void setSprite(const spk::SpriteSheet::Sprite &p_sprite)
		{
			_sprite = p_sprite;
		}
	};
}
