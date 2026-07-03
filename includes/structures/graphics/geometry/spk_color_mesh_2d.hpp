#pragma once

#include <functional>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/geometry/spk_generic_mesh.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	struct ColorVertex2D
	{
		spk::Vector3 position{};
		spk::Color color{};

		[[nodiscard]] bool operator==(const ColorVertex2D &p_other) const noexcept
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
		std::size_t operator()(const spk::ColorVertex2D &p_value) const noexcept
		{
			std::size_t h = std::hash<float>{}(p_value.position.x);
			h ^= std::hash<float>{}(p_value.position.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.position.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
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
	using ColorMesh2DLayout = spk::MeshLayout<
		spk::LayoutBufferObject::Attribute{0, spk::LayoutBufferObject::Attribute::Type::Vector3},
		spk::LayoutBufferObject::Attribute{1, spk::LayoutBufferObject::Attribute::Type::Vector4}>;
	using ColorMesh2D = spk::GenericMesh<spk::ColorVertex2D, spk::ColorMesh2DLayout>;
}
