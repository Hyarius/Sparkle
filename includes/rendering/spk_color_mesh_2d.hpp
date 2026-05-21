#pragma once

#include <functional>

#include "rendering/spk_color.hpp"
#include "rendering/spk_generic_mesh.hpp"
#include "math/spk_vector2.hpp"

namespace spk
{
	struct ColorVertex2D
	{
		spk::Vector2 position{};
		spk::Color color{};

		[[nodiscard]] bool operator==(const ColorVertex2D& p_other) const noexcept
		{
			return position == p_other.position &&
				   color.r == p_other.color.r &&
				   color.g == p_other.color.g &&
				   color.b == p_other.color.b &&
				   color.a == p_other.color.a;
		}
	};

}

namespace std
{
	template <>
	struct hash<spk::ColorVertex2D>
	{
		std::size_t operator()(const spk::ColorVertex2D& p_value) const noexcept
		{
			std::size_t h = std::hash<spk::Vector2>{}(p_value.position);
			h ^= std::hash<float>{}(p_value.color.r) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.color.g) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.color.b) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.color.a) + 0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
}

namespace spk
{
	class ColorMesh2D : public spk::GenericMesh<spk::ColorVertex2D>
	{
	public:
		ColorMesh2D() = default;
	};
}
