#pragma once

#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/math/spk_vector3.hpp"
#include "voxel/voxel_shape.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pg
{
	class JsonReader;

	struct ModelBox
	{
		spk::Vector3 minimum{};
		spk::Vector3 maximum{};
		AtlasCell texture{};
	};

	struct ModelPart
	{
		std::string logicalPart;
		spk::Vector3 origin{};
		std::vector<ModelBox> boxes;
		std::shared_ptr<const spk::TextureMesh3D> mesh;
	};

	struct ModelDefinition
	{
		std::string id;
		std::vector<ModelPart> parts;

		[[nodiscard]] const ModelPart *part(const std::string &p_logicalPart) const noexcept;
		[[nodiscard]] static ModelDefinition placeholderCube();
	};

	[[nodiscard]] ModelDefinition parseModelDefinition(JsonReader &p_reader);
}
