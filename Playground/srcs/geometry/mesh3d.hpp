#pragma once

#include <cstddef>
#include <functional>

#include "structures/graphics/geometry/spk_generic_mesh.hpp"
#include "structures/math/spk_vector2.hpp"
#include "structures/math/spk_vector3.hpp"

namespace pg
{
	struct MeshVertex3D
	{
		spk::Vector3 position{};
		spk::Vector3 normal{};
		spk::Vector2 uv{};

		[[nodiscard]] bool operator==(const MeshVertex3D &p_other) const noexcept
		{
			return position == p_other.position && normal == p_other.normal && uv == p_other.uv;
		}
	};
}

namespace std
{
	template <>
	struct hash<pg::MeshVertex3D>
	{
		std::size_t operator()(const pg::MeshVertex3D &p_value) const noexcept
		{
			std::size_t hash = std::hash<float>{}(p_value.position.x);
			hash ^= std::hash<float>{}(p_value.position.y) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			hash ^= std::hash<float>{}(p_value.position.z) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			hash ^= std::hash<float>{}(p_value.normal.x) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			hash ^= std::hash<float>{}(p_value.normal.y) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			hash ^= std::hash<float>{}(p_value.normal.z) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			hash ^= std::hash<spk::Vector2>{}(p_value.uv) + 0x9e3779b9u + (hash << 6) + (hash >> 2);
			return hash;
		}
	};
}

namespace pg
{
	class Mesh3D : public spk::GenericMesh<MeshVertex3D>
	{
	public:
		Mesh3D()
		{
			_layoutBuffer.addAttribute(0, spk::LayoutBufferObject::Attribute::Type::Vector3);
			_layoutBuffer.addAttribute(1, spk::LayoutBufferObject::Attribute::Type::Vector3);
			_layoutBuffer.addAttribute(2, spk::LayoutBufferObject::Attribute::Type::Vector2);
		}
	};
}
