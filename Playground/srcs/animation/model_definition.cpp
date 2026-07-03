#include "animation/model_definition.hpp"

#include "core/json.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>

namespace
{
	[[nodiscard]] spk::Vector3 vector3(
		const nlohmann::json &p_json,
		const std::filesystem::path &p_file,
		const std::string &p_path)
	{
		if (!p_json.is_array() || p_json.size() != 3 ||
			!p_json[0].is_number() || !p_json[1].is_number() || !p_json[2].is_number())
		{
			throw pg::JsonError(p_file, p_path, "expected an array of three numbers");
		}
		spk::Vector3 result{p_json[0].get<float>(), p_json[1].get<float>(), p_json[2].get<float>()};
		if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z))
		{
			throw pg::JsonError(p_file, p_path, "coordinates must be finite");
		}
		return result;
	}

	[[nodiscard]] pg::AtlasCell atlasCell(
		const nlohmann::json &p_json,
		const std::filesystem::path &p_file,
		const std::string &p_path)
	{
		if (!p_json.is_array() || p_json.size() != 2 || !p_json[0].is_number_integer() || !p_json[1].is_number_integer())
		{
			throw pg::JsonError(p_file, p_path, "expected an array of two integers");
		}
		pg::AtlasCell result{p_json[0].get<int>(), p_json[1].get<int>()};
		if (result.column < 0 || result.column >= pg::VoxelShape::AtlasColumns || result.row < 0 || result.row >= pg::VoxelShape::AtlasRows)
		{
			throw pg::JsonError(p_file, p_path, "texture coordinate is outside the 8x8 atlas");
		}
		return result;
	}

	void addFace(
		spk::TextureMesh3D::Builder &p_builder,
		const std::array<spk::Vector3, 4> &p_positions,
		const spk::Vector3 &p_normal,
		const pg::AtlasCell &p_texture)
	{
		const std::array<spk::Vector2, 4> localUvs = {
			spk::Vector2{0, 0}, spk::Vector2{1, 0}, spk::Vector2{1, 1}, spk::Vector2{0, 1}};
		std::array<spk::TextureVertex3D, 4> vertices;
		for (std::size_t index = 0; index < vertices.size(); ++index)
		{
			vertices[index] = {p_positions[index], p_normal, pg::VoxelShape::atlasUV(p_texture, localUvs[index])};
		}
		p_builder.addShape(vertices);
	}

	[[nodiscard]] std::shared_ptr<const spk::TextureMesh3D> buildMesh(
		const std::vector<pg::ModelBox> &p_boxes,
		const spk::Vector3 &p_offset)
	{
		spk::TextureMesh3D::Builder builder;
		builder.reserve(p_boxes.size() * 24, p_boxes.size() * 36);
		for (const pg::ModelBox &box : p_boxes)
		{
			const spk::Vector3 a = box.minimum + p_offset;
			const spk::Vector3 b = box.maximum + p_offset;
			addFace(builder, {{{b.x, a.y, b.z}, {b.x, a.y, a.z}, {b.x, b.y, a.z}, {b.x, b.y, b.z}}}, {1, 0, 0}, box.texture);
			addFace(builder, {{{a.x, a.y, a.z}, {a.x, a.y, b.z}, {a.x, b.y, b.z}, {a.x, b.y, a.z}}}, {-1, 0, 0}, box.texture);
			addFace(builder, {{{a.x, b.y, b.z}, {b.x, b.y, b.z}, {b.x, b.y, a.z}, {a.x, b.y, a.z}}}, {0, 1, 0}, box.texture);
			addFace(builder, {{{a.x, a.y, a.z}, {b.x, a.y, a.z}, {b.x, a.y, b.z}, {a.x, a.y, b.z}}}, {0, -1, 0}, box.texture);
			addFace(builder, {{{a.x, a.y, b.z}, {b.x, a.y, b.z}, {b.x, b.y, b.z}, {a.x, b.y, b.z}}}, {0, 0, 1}, box.texture);
			addFace(builder, {{{b.x, a.y, a.z}, {a.x, a.y, a.z}, {a.x, b.y, a.z}, {b.x, b.y, a.z}}}, {0, 0, -1}, box.texture);
		}
		return std::make_shared<spk::TextureMesh3D>(builder.bake());
	}

	void buildCenteredPartMeshes(pg::ModelDefinition &p_model)
	{
		float minimumX = std::numeric_limits<float>::max();
		float maximumX = std::numeric_limits<float>::lowest();
		float minimumY = std::numeric_limits<float>::max();
		float maximumY = std::numeric_limits<float>::lowest();
		float minimumZ = std::numeric_limits<float>::max();
		float maximumZ = std::numeric_limits<float>::lowest();
		for (const pg::ModelPart &part : p_model.parts)
		{
			for (const pg::ModelBox &box : part.boxes)
			{
				minimumX = std::min(minimumX, box.minimum.x - part.origin.x);
				maximumX = std::max(maximumX, box.maximum.x - part.origin.x);
				minimumY = std::min(minimumY, box.minimum.y - part.origin.y);
				maximumY = std::max(maximumY, box.maximum.y - part.origin.y);
				minimumZ = std::min(minimumZ, box.minimum.z - part.origin.z);
				maximumZ = std::max(maximumZ, box.maximum.z - part.origin.z);
			}
		}

		// The battle-unit root is placed at the center of its occupied volume: cell center on
		// X/Z and half a unit above the walk surface on Y. Center the complete model around it.
		const spk::Vector3 offset{
			-(minimumX + maximumX) * 0.5f,
			-(minimumY + maximumY) * 0.5f,
			-(minimumZ + maximumZ) * 0.5f};
		for (pg::ModelPart &part : p_model.parts)
		{
			part.mesh = buildMesh(part.boxes, offset);
		}
	}
}

namespace pg
{
	const ModelPart *ModelDefinition::part(const std::string &p_logicalPart) const noexcept
	{
		const auto found = std::ranges::find_if(parts, [&](const ModelPart &p_part) {
			return p_part.logicalPart == p_logicalPart;
		});
		return found == parts.end() ? nullptr : &*found;
	}

	ModelDefinition ModelDefinition::placeholderCube()
	{
		ModelDefinition result{.id = "placeholder-cube"};
		ModelPart part{.logicalPart = "wholeRig"};
		part.boxes.push_back({.minimum = {0, 0, 0}, .maximum = {1, 1, 1}, .texture = {4, 0}});
		result.parts.push_back(std::move(part));
		buildCenteredPartMeshes(result);
		return result;
	}

	ModelDefinition parseModelDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "parts"});
		if (p_reader.require<int>("version") != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported model version");
		}
		const std::vector<JsonReader> partReaders = p_reader.childArray("parts");
		if (partReaders.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("parts"), "model must contain at least one part");
		}

		ModelDefinition result;
		std::set<std::string> logicalParts;
		for (JsonReader partReader : partReaders)
		{
			partReader.forbidUnknown({"logicalPart", "origin", "voxels"});
			ModelPart part;
			part.logicalPart = partReader.require<std::string>("logicalPart");
			if (part.logicalPart.empty() || !logicalParts.insert(part.logicalPart).second)
			{
				throw JsonError(partReader.file(), partReader.pathFor("logicalPart"), "logicalPart must be non-empty and unique");
			}
			part.origin = vector3(
				partReader.require<nlohmann::json>("origin"), partReader.file(), partReader.pathFor("origin"));
			const std::vector<JsonReader> boxes = partReader.childArray("voxels");
			if (boxes.empty())
			{
				throw JsonError(partReader.file(), partReader.pathFor("voxels"), "part must contain at least one box");
			}
			for (JsonReader boxReader : boxes)
			{
				boxReader.forbidUnknown({"min", "max", "textures"});
				ModelBox box;
				box.minimum = vector3(
					boxReader.require<nlohmann::json>("min"), boxReader.file(), boxReader.pathFor("min"));
				box.maximum = vector3(
					boxReader.require<nlohmann::json>("max"), boxReader.file(), boxReader.pathFor("max"));
				if (box.maximum.x <= box.minimum.x || box.maximum.y <= box.minimum.y || box.maximum.z <= box.minimum.z)
				{
					throw JsonError(boxReader.file(), boxReader.pathFor("max"), "box max must be greater than min on every axis");
				}
				JsonReader textures = boxReader.child("textures");
				textures.forbidUnknown({"all"});
				box.texture = atlasCell(
					textures.require<nlohmann::json>("all"), textures.file(), textures.pathFor("all"));
				part.boxes.push_back(box);
			}
			result.parts.push_back(std::move(part));
		}
		buildCenteredPartMeshes(result);
		return result;
	}
}
