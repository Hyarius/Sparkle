#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "structures/graphics/geometry/spk_generic_mesh.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace spk
{
	struct TextureVertex2D
	{
		spk::Vector3 position{};
		spk::Vector2 uv{};

		[[nodiscard]] bool operator==(const TextureVertex2D &p_other) const noexcept
		{
			return position == p_other.position && uv == p_other.uv;
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::TextureVertex2D>
	{
		std::size_t operator()(const spk::TextureVertex2D &p_value) const noexcept
		{
			std::size_t h = std::hash<float>{}(p_value.position.x);
			h ^= std::hash<float>{}(p_value.position.y) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<float>{}(p_value.position.z) + 0x9e3779b9u + (h << 6) + (h >> 2);
			h ^= std::hash<spk::Vector2>{}(p_value.uv) + 0x9e3779b9u + (h << 6) + (h >> 2);
			return h;
		}
	};
}

namespace spk
{
	using TextureMesh2DLayout = spk::MeshLayout<
		spk::LayoutBufferObject::Attribute{0, spk::LayoutBufferObject::Attribute::Type::Vector3},
		spk::LayoutBufferObject::Attribute{1, spk::LayoutBufferObject::Attribute::Type::Vector2}>;
	using TextureMesh2D = spk::GenericMesh<spk::TextureVertex2D, spk::TextureMesh2DLayout>;
}
