#include "voxel/shape_catalog.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
	// Authoring tolerance: positions may overshoot the unit cell by the mesher's boundary
	// epsilon (spk::DataVoxelShape snaps them back onto the boundary).
	constexpr float PositionTolerance = spk::VoxelShape::BoundaryEpsilon;

	spk::Vector3 readPosition(const pg::JsonReader &p_polygonReader, const std::array<float, 3> &p_value, std::size_t p_index)
	{
		for (const float coordinate : p_value)
		{
			if (coordinate < -PositionTolerance || coordinate > 1.0f + PositionTolerance)
			{
				throw pg::JsonError(
					p_polygonReader.file(),
					p_polygonReader.pathFor("positions") + "[" + std::to_string(p_index) + "]",
					"vertex positions must stay inside the unit voxel cell (0..1 on every axis)");
			}
		}
		return {p_value[0], p_value[1], p_value[2]};
	}

	spk::Vector2 readUV(const pg::JsonReader &p_polygonReader, const std::array<float, 2> &p_value, std::size_t p_index)
	{
		for (const float coordinate : p_value)
		{
			if (coordinate < 0.0f || coordinate > 1.0f)
			{
				throw pg::JsonError(
					p_polygonReader.file(),
					p_polygonReader.pathFor("uvs") + "[" + std::to_string(p_index) + "]",
					"UV coordinates must be between 0 and 1 (local to the atlas cell)");
			}
		}
		return {p_value[0], p_value[1]};
	}

	spk::VoxelShapeDescription::Polygon parsePolygon(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"slot", "positions", "uvs"});

		spk::VoxelShapeDescription::Polygon polygon;
		polygon.slot = p_reader.require<std::string>("slot");
		if (polygon.slot.empty())
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("slot"), "texture slot name cannot be empty");
		}

		const auto positions = p_reader.require<std::vector<std::array<float, 3>>>("positions");
		const auto uvs = p_reader.require<std::vector<std::array<float, 2>>>("uvs");
		if (positions.size() < 3)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("positions"), "a polygon requires at least three vertices");
		}
		if (uvs.size() != positions.size())
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("uvs"),
				"expected one UV per vertex (" + std::to_string(positions.size()) + " positions, " +
					std::to_string(uvs.size()) + " uvs)");
		}

		polygon.positions.reserve(positions.size());
		polygon.uvs.reserve(uvs.size());
		for (std::size_t index = 0; index < positions.size(); ++index)
		{
			polygon.positions.push_back(readPosition(p_reader, positions[index], index));
			polygon.uvs.push_back(readUV(p_reader, uvs[index], index));
		}
		return polygon;
	}

	// A height entry is either one number (uniform in every direction) or an object naming
	// all five directional heights.
	pg::CardinalHeightSet parseHeightSet(const pg::JsonReader &p_heightsReader, const std::string &p_key)
	{
		const spk::JSON::Value *member = p_heightsReader.value().find(p_key);
		if (member != nullptr && member->isObject())
		{
			pg::JsonReader setReader = p_heightsReader.child(p_key);
			setReader.forbidUnknown({"positiveX", "negativeX", "positiveZ", "negativeZ", "stationary"});
			return {
				setReader.require<float>("positiveX"),
				setReader.require<float>("negativeX"),
				setReader.require<float>("positiveZ"),
				setReader.require<float>("negativeZ"),
				setReader.require<float>("stationary")};
		}

		const float uniform = p_heightsReader.require<float>(p_key);
		return {uniform, uniform, uniform, uniform, uniform};
	}
}

namespace pg
{
	std::unique_ptr<spk::VoxelShape> ShapeDefinition::instantiate(spk::VoxelShape::TextureSlots p_textures) const
	{
		return std::make_unique<spk::DataVoxelShape>(std::move(p_textures), description);
	}

	ShapeDefinition parseShapeDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "polygons", "heights"});

		const int version = p_reader.require<int>("version");
		if (version != 1)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version " + std::to_string(version) + " (expected 1)");
		}

		ShapeDefinition definition;
		std::vector<JsonReader> polygonReaders = p_reader.childArray("polygons");
		if (polygonReaders.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("polygons"), "a shape requires at least one polygon");
		}
		definition.description.polygons.reserve(polygonReaders.size());
		for (JsonReader &polygonReader : polygonReaders)
		{
			spk::VoxelShapeDescription::Polygon polygon = parsePolygon(polygonReader);
			definition.slots.insert(polygon.slot);
			definition.description.polygons.push_back(std::move(polygon));
		}

		// Omitted heights (or an omitted flip) mean "not walkable in that orientation":
		// all-zero heights, the profile of pass-through scenery like flowers and leaves.
		if (p_reader.contains("heights"))
		{
			JsonReader heightsReader = p_reader.child("heights");
			heightsReader.forbidUnknown({"positiveY", "negativeY"});
			if (heightsReader.contains("positiveY"))
			{
				definition.heights.positiveY = parseHeightSet(heightsReader, "positiveY");
			}
			if (heightsReader.contains("negativeY"))
			{
				definition.heights.negativeY = parseHeightSet(heightsReader, "negativeY");
			}
		}

		return definition;
	}
}
