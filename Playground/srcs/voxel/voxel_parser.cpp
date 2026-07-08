#include "voxel/voxel_definition.hpp"

#include "core/json.hpp"
#include "voxel/atlas_cell.hpp"

#include "structures/voxel/spk_cross_plane_voxel_shape.hpp"
#include "structures/voxel/spk_cross_voxel_shape.hpp"
#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_cuboid_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"
#include "structures/voxel/spk_slope_voxel_shape.hpp"
#include "structures/voxel/spk_stair_voxel_shape.hpp"

#include <array>
#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace
{
	constexpr int AtlasColumns = 8;
	constexpr int AtlasRows = 8;

	pg::AtlasCell readAtlasCell(const pg::JsonReader &p_reader, const std::string &p_slot)
	{
		const std::array<int, 2> value = p_reader.require<std::array<int, 2>>(p_slot);
		if (value[0] < 0 || value[0] >= AtlasColumns || value[1] < 0 || value[1] >= AtlasRows)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_slot), "atlas cell must be inside the 8x8 voxel atlas");
		}
		return {value[0], value[1]};
	}

	spk::VoxelShape::TextureSlots readCubeTextures(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"top", "bottom", "side", "posX", "negX", "posZ", "negZ"});
		spk::VoxelShape::TextureSlots result;
		result.emplace("top", readAtlasCell(p_reader, "top"));
		result.emplace("bottom", readAtlasCell(p_reader, "bottom"));

		const bool hasSide = p_reader.contains("side");
		const bool hasDirectional = p_reader.contains("posX") || p_reader.contains("negX") ||
									p_reader.contains("posZ") || p_reader.contains("negZ");
		if (hasSide && hasDirectional)
		{
			throw pg::JsonError(p_reader.file(), p_reader.path(), "use either 'side' or all four directional side slots");
		}

		if (hasSide)
		{
			const pg::AtlasCell side = readAtlasCell(p_reader, "side");
			result.emplace("posX", side);
			result.emplace("negX", side);
			result.emplace("posZ", side);
			result.emplace("negZ", side);
		}
		else
		{
			result.emplace("posX", readAtlasCell(p_reader, "posX"));
			result.emplace("negX", readAtlasCell(p_reader, "negX"));
			result.emplace("posZ", readAtlasCell(p_reader, "posZ"));
			result.emplace("negZ", readAtlasCell(p_reader, "negZ"));
		}
		return result;
	}

	spk::VoxelShape::TextureSlots readSideTextures(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"top", "bottom", "side"});
		spk::VoxelShape::TextureSlots result;
		result.emplace("top", readAtlasCell(p_reader, "top"));
		result.emplace("bottom", readAtlasCell(p_reader, "bottom"));
		const pg::AtlasCell side = readAtlasCell(p_reader, "side");
		result.emplace("posX", side);
		result.emplace("negX", side);
		result.emplace("posZ", side);
		result.emplace("negZ", side);
		return result;
	}

	spk::VoxelShape::TextureSlots readNamedTextures(
		pg::JsonReader &p_reader,
		std::initializer_list<std::string_view> p_names)
	{
		p_reader.forbidUnknown(p_names);
		spk::VoxelShape::TextureSlots result;
		for (const std::string_view name : p_names)
		{
			const std::string key(name);
			result.emplace(key, readAtlasCell(p_reader, key));
		}
		return result;
	}

	// Uniform walk heights (a full-height top all around) - the default for solid shapes.
	pg::CardinalHeightSet flatTop()
	{
		return {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	}

	pg::CardinalHeightSet flatTop(float p_height)
	{
		return {p_height, p_height, p_height, p_height, p_height};
	}

	spk::Vector3 readUnitVector(pg::JsonReader &p_reader, const std::string &p_slot)
	{
		const std::array<float, 3> value = p_reader.require<std::array<float, 3>>(p_slot);
		return {value[0], value[1], value[2]};
	}

	// Walk-height formulas ported verbatim from the historical pg::*Shape::_constructHeights so
	// the navigation graph keeps stepping up and down exactly as before.
	struct ParsedShape
	{
		std::unique_ptr<spk::VoxelShape> shape;
		pg::CardinalHeightCollection heights;
	};

	ParsedShape parseShape(pg::JsonReader &p_reader)
	{
		const std::string type = p_reader.require<std::string>("type");

		if (type == "cube")
		{
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return {std::make_unique<spk::CubeVoxelShape>(readCubeTextures(textures)), {flatTop(), flatTop()}};
		}
		if (type == "cuboid")
		{
			p_reader.forbidUnknown({"type", "textures", "min", "max"});
			const spk::Vector3 minimum = readUnitVector(p_reader, "min");
			const spk::Vector3 maximum = readUnitVector(p_reader, "max");
			if (minimum.x < 0.0f || minimum.y < 0.0f || minimum.z < 0.0f ||
				maximum.x > 1.0f || maximum.y > 1.0f || maximum.z > 1.0f ||
				minimum.x >= maximum.x || minimum.y >= maximum.y || minimum.z >= maximum.z)
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor("max"),
					"cuboid min/max must define a positive box inside the unit voxel cell");
			}
			pg::JsonReader textures = p_reader.child("textures");
			return {
				std::make_unique<spk::CuboidVoxelShape>(readCubeTextures(textures), minimum, maximum),
				{flatTop(maximum.y), flatTop(1.0f - minimum.y)}};
		}
		if (type == "slab")
		{
			p_reader.forbidUnknown({"type", "textures", "height"});
			const float height = p_reader.optional<float>("height", 0.5f);
			if (height < 0.0f || height > 1.0f)
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor("height"), "height must be between 0 and 1");
			}
			pg::JsonReader textures = p_reader.child("textures");
			const pg::CardinalHeightSet top{height, height, height, height, height};
			return {std::make_unique<spk::SlabVoxelShape>(readSideTextures(textures), height), {top, flatTop()}};
		}
		if (type == "slope")
		{
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			const pg::CardinalHeightSet top{0.5f, 0.5f, 1.0f, 0.0f, 0.5f};
			return {
				std::make_unique<spk::SlopeVoxelShape>(readNamedTextures(textures, {"slope", "back", "bottom", "sideLeft", "sideRight"})),
				{top, flatTop()}};
		}
		if (type == "stair")
		{
			p_reader.forbidUnknown({"type", "textures", "stepCount"});
			const int stepCount = p_reader.optional<int>("stepCount", 2);
			if (stepCount <= 0 || stepCount > 64)
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor("stepCount"), "stepCount must be between 1 and 64");
			}
			pg::JsonReader textures = p_reader.child("textures");
			const float firstStepMidpoint = 1.0f / (2.0f * static_cast<float>(stepCount));
			const pg::CardinalHeightSet top{0.5f, 0.5f, 1.0f, firstStepMidpoint, 0.5f};
			return {
				std::make_unique<spk::StairVoxelShape>(
					readNamedTextures(textures, {"top", "riser", "back", "bottom", "sideLeft", "sideRight"}), stepCount),
				{top, flatTop()}};
		}
		if (type == "crossPlane")
		{
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return {std::make_unique<spk::DiagonalCrossVoxelShape>(readNamedTextures(textures, {"plane"})), {}};
		}
		if (type == "cross")
		{
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return {std::make_unique<spk::CrossVoxelShape>(readNamedTextures(textures, {"plane"})), {}};
		}

		throw pg::JsonError(
			p_reader.file(),
			p_reader.pathFor("type"),
			"unknown voxel shape type '" + type + "' (known types: cube, cuboid, slab, slope, stair, crossPlane, cross)");
	}
}

namespace pg
{
	ParsedVoxel parseVoxelDefinition(JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"version", "traversal", "tags", "shape"});

		const int version = p_reader.require<int>("version");
		if (version != 1)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version " + std::to_string(version) + " (expected 1)");
		}

		static const std::map<std::string, VoxelTraversal> traversalValues = {
			{"passable", VoxelTraversal::Passable},
			{"solid", VoxelTraversal::Solid}};

		ParsedVoxel result;
		result.data.traversal = p_reader.requireEnum<VoxelTraversal>("traversal", traversalValues);
		result.data.tags = p_reader.require<std::vector<std::string>>("tags");
		JsonReader shapeReader = p_reader.child("shape");
		ParsedShape parsed = parseShape(shapeReader);
		result.shape = std::move(parsed.shape);
		result.heights = parsed.heights;
		return result;
	}
}
