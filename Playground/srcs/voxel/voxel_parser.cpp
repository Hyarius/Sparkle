#include "voxel/voxel_definition.hpp"

#include "core/json.hpp"
#include "core/registry.hpp"
#include "voxel/shapes/cross_plane_shape.hpp"
#include "voxel/shapes/cube_shape.hpp"
#include "voxel/shapes/slab_shape.hpp"
#include "voxel/shapes/slope_shape.hpp"
#include "voxel/shapes/stair_shape.hpp"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
	pg::AtlasCell readAtlasCell(const pg::JsonReader &p_reader, const std::string &p_slot)
	{
		const std::array<int, 2> value = p_reader.require<std::array<int, 2>>(p_slot);
		if (value[0] < 0 || value[0] >= pg::VoxelShape::AtlasColumns ||
			value[1] < 0 || value[1] >= pg::VoxelShape::AtlasRows)
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor(p_slot),
				"atlas cell must be inside the 8x8 voxel atlas");
		}
		return {value[0], value[1]};
	}

	pg::VoxelShape::TextureSlots readCubeTextures(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"top", "bottom", "side", "posX", "negX", "posZ", "negZ"});
		pg::VoxelShape::TextureSlots result;
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

	pg::VoxelShape::TextureSlots readSideTextures(pg::JsonReader &p_reader)
	{
		p_reader.forbidUnknown({"top", "bottom", "side"});
		pg::VoxelShape::TextureSlots result;
		result.emplace("top", readAtlasCell(p_reader, "top"));
		result.emplace("bottom", readAtlasCell(p_reader, "bottom"));
		const pg::AtlasCell side = readAtlasCell(p_reader, "side");
		result.emplace("posX", side);
		result.emplace("negX", side);
		result.emplace("posZ", side);
		result.emplace("negZ", side);
		return result;
	}

	pg::VoxelShape::TextureSlots readNamedTextures(
		pg::JsonReader &p_reader,
		std::initializer_list<std::string_view> p_names)
	{
		p_reader.forbidUnknown(p_names);
		pg::VoxelShape::TextureSlots result;
		for (const std::string_view name : p_names)
		{
			const std::string key(name);
			result.emplace(key, readAtlasCell(p_reader, key));
		}
		return result;
	}

	void registerShapeParsers(pg::PolymorphicFactory<pg::VoxelShape> &p_factory)
	{
		p_factory.registerType("cube", [](pg::JsonReader &p_reader) -> std::unique_ptr<pg::VoxelShape> {
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return std::make_unique<pg::CubeShape>(readCubeTextures(textures));
		});

		p_factory.registerType("slab", [](pg::JsonReader &p_reader) -> std::unique_ptr<pg::VoxelShape> {
			p_reader.forbidUnknown({"type", "textures", "height"});
			const float height = p_reader.optional<float>("height", 0.5f);
			if (height < 0.0f || height > 1.0f)
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor("height"), "height must be between 0 and 1");
			}
			pg::JsonReader textures = p_reader.child("textures");
			return std::make_unique<pg::SlabShape>(readSideTextures(textures), height);
		});

		p_factory.registerType("slope", [](pg::JsonReader &p_reader) -> std::unique_ptr<pg::VoxelShape> {
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return std::make_unique<pg::SlopeShape>(readNamedTextures(
				textures, {"slope", "back", "bottom", "sideLeft", "sideRight"}));
		});

		p_factory.registerType("stair", [](pg::JsonReader &p_reader) -> std::unique_ptr<pg::VoxelShape> {
			p_reader.forbidUnknown({"type", "textures", "stepCount"});
			const int stepCount = p_reader.optional<int>("stepCount", 2);
			if (stepCount <= 0 || stepCount > 64)
			{
				throw pg::JsonError(p_reader.file(), p_reader.pathFor("stepCount"), "stepCount must be between 1 and 64");
			}
			pg::JsonReader textures = p_reader.child("textures");
			return std::make_unique<pg::StairShape>(readNamedTextures(textures, {"top", "riser", "back", "bottom", "sideLeft", "sideRight"}), stepCount);
		});

		p_factory.registerType("crossPlane", [](pg::JsonReader &p_reader) -> std::unique_ptr<pg::VoxelShape> {
			p_reader.forbidUnknown({"type", "textures"});
			pg::JsonReader textures = p_reader.child("textures");
			return std::make_unique<pg::CrossPlaneShape>(readNamedTextures(textures, {"plane"}));
		});
	}

	pg::PolymorphicFactory<pg::VoxelShape> &shapeFactory()
	{
		static pg::PolymorphicFactory<pg::VoxelShape> factory;
		static const bool registered = []() {
			registerShapeParsers(factory);
			return true;
		}();
		(void)registered;
		return factory;
	}
}

namespace pg
{
	VoxelDefinition parseVoxelDefinition(JsonReader &p_reader)
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

		VoxelDefinition result;
		result.data.traversal = p_reader.requireEnum<VoxelTraversal>("traversal", traversalValues);
		result.data.tags = p_reader.require<std::vector<std::string>>("tags");
		JsonReader shapeReader = p_reader.child("shape");
		result.shape = shapeFactory().parse(shapeReader);
		result.shape->initialize();
		return result;
	}
}
