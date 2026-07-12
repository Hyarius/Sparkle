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
		bool _castsShadows = true;
		bool _receivesShadows = true;
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

		void setCastsShadows(bool p_value) noexcept
		{
			_castsShadows = p_value;
		}

		void setReceivesShadows(bool p_value) noexcept
		{
			_receivesShadows = p_value;
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

		[[nodiscard]] bool castsShadows() const noexcept
		{
			return _castsShadows;
		}

		[[nodiscard]] bool receivesShadows() const noexcept
		{
			return _receivesShadows;
		}

		[[nodiscard]] const spk::Color &tint() const
		{
			return _tint;
		}
	};
}
