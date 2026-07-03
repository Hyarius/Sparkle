#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/graphics/spk_texture.hpp"

#include <memory>
#include <utility>

namespace spk
{
	class TextureMeshRenderer3D : public spk::Component
	{
	private:
		std::shared_ptr<const spk::TextureMesh3D> _mesh;
		const spk::Texture *_texture = nullptr;
		bool _translucent = false;
		spk::Color _tint;

	public:
		void setMesh(std::shared_ptr<const spk::TextureMesh3D> p_mesh)
		{
			_mesh = std::move(p_mesh);
		}

		void setTexture(const spk::Texture *p_texture)
		{
			_texture = p_texture;
		}

		void setTranslucent(bool p_translucent)
		{
			_translucent = p_translucent;
		}

		void setTint(const spk::Color &p_tint)
		{
			_tint = p_tint;
		}

		[[nodiscard]] const std::shared_ptr<const spk::TextureMesh3D> &mesh() const
		{
			return _mesh;
		}

		[[nodiscard]] const spk::Texture *texture() const
		{
			return _texture;
		}

		[[nodiscard]] bool translucent() const
		{
			return _translucent;
		}

		[[nodiscard]] const spk::Color &tint() const
		{
			return _tint;
		}
	};
}
