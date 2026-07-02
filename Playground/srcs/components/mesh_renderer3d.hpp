#pragma once

#include "geometry/mesh3d.hpp"
#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/spk_texture.hpp"

#include <memory>

namespace pg
{
	// Data-only component read by MeshRenderLogic alongside the owning Transform3D.
	//
	// The mesh is held by shared_ptr and treated as immutable once built: to change the
	// geometry, build a NEW mesh and setMesh() it rather than mutating in place. This is
	// what makes rendering thread-safe — a render command captures a shared_ptr copy, so the
	// mesh it draws on the render thread stays alive even if the game swaps in a new mesh on
	// the update thread the same frame (see MeshRenderCommand / the multithreaded render loop).
	class MeshRenderer3D : public spk::Component
	{
	private:
		std::shared_ptr<const Mesh3D> _mesh;
		const spk::Texture *_texture = nullptr;
		bool _translucent = false;
		spk::Color _tint;

	public:
		void setMesh(std::shared_ptr<const Mesh3D> p_mesh)
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

		[[nodiscard]] const std::shared_ptr<const Mesh3D> &mesh() const
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
