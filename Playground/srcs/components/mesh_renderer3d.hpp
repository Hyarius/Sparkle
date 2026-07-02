#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_2d.hpp"
#include "structures/graphics/spk_texture.hpp"

namespace pg
{
	// Data-only component: a mesh + a texture to draw it with. MeshRenderLogic reads
	// these plus the owning entity's Transform3D to emit a MeshRenderCommand.
	//
	// NOTE: spk::TextureMesh2D already stores Vector3 positions + UV with GL layout
	// attributes, so it doubles as the Step-1 3D mesh. A dedicated pg::Mesh3D carrying
	// normals arrives in Step 2 (for lighting / voxels).
	class MeshRenderer3D : public spk::Component
	{
	private:
		const spk::TextureMesh2D *_mesh = nullptr;
		const spk::Texture *_texture = nullptr;

	public:
		void setMesh(const spk::TextureMesh2D *p_mesh) { _mesh = p_mesh; }
		void setTexture(const spk::Texture *p_texture) { _texture = p_texture; }

		[[nodiscard]] const spk::TextureMesh2D *mesh() const { return _mesh; }
		[[nodiscard]] const spk::Texture *texture() const { return _texture; }
	};
}
