#pragma once

#include <functional>

#include "structures/graphics/geometry/spk_generic_mesh.hpp"
#include "structures/math/spk_vector2.hpp"

namespace spk
{
	struct Vertex2D
	{
		spk::Vector2 position{};
		spk::Vector2 uv{};

		[[nodiscard]] bool operator==(const Vertex2D& p_other) const noexcept
		{
			return position == p_other.position && uv == p_other.uv;
		}
	};

}

namespace std
{
	template <>
	struct hash<spk::Vertex2D>
	{
		std::size_t operator()(const spk::Vertex2D& p_value) const noexcept
		{
			std::size_t h = std::hash<spk::Vector2>{}(p_value.position);
			h ^= std::hash<spk::Vector2>{}(p_value.uv) + 0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
}

namespace spk
{
	class Mesh2D : public spk::GenericMesh<spk::Vertex2D>
	{
	public:
		Mesh2D() = default;
	};
}
