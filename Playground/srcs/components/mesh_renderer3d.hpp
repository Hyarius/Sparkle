#pragma once

#include "geometry/mesh3d.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace pg
{
	// Data-only component read by MeshRenderLogic alongside the owning Transform3D.
	class MeshRenderer3D : public spk::Component
	{
	private:
		const Mesh3D *_mesh = nullptr;
		const spk::Texture *_texture = nullptr;
		bool _translucent = false;
		spk::Color _tint;

	public:
		void setMesh(const Mesh3D *p_mesh)
		{
			_mesh = p_mesh;
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

		[[nodiscard]] const Mesh3D *mesh() const
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
