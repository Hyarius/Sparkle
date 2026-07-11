#include "voxel/voxel_definition.hpp"

#include "core/json.hpp"

#include <array>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
	constexpr int AtlasColumns = 8;
	constexpr int AtlasRows = 8;

	spk::AtlasCell readAtlasCell(const pg::JsonReader &p_reader, const std::string &p_slot)
	{
		const std::array<int, 2> value = p_reader.require<std::array<int, 2>>(p_slot);
		if (value[0] < 0 || value[0] >= AtlasColumns || value[1] < 0 || value[1] >= AtlasRows)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor(p_slot), "atlas cell must be inside the 8x8 voxel atlas");
		}
		return {value[0], value[1]};
	}

	// The voxel must provide exactly the texture slots its shape's polygons reference: a
	// missing slot would leave geometry untextured, an extra one is a typo worth flagging.
	spk::VoxelShape::TextureSlots readTextures(pg::JsonReader &p_reader, const pg::ShapeDefinition &p_shape)
	{
		spk::VoxelShape::TextureSlots result;
		for (const std::string &slot : p_shape.slots)
		{
			if (!p_reader.contains(slot))
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor(slot),
					"shape '" + p_shape.id + "' requires texture slot '" + slot + "'");
			}
			result.emplace(slot, readAtlasCell(p_reader, slot));
		}

		for (const auto &[key, unused] : p_reader.value().asObject())
		{
			(void)unused;
			if (!p_shape.slots.contains(key))
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor(key),
					"texture slot '" + key + "' is not used by shape '" + p_shape.id + "'");
			}
		}
		return result;
	}

	const pg::ShapeDefinition &resolveShape(const pg::JsonReader &p_reader, const pg::ShapeCatalog &p_shapes)
	{
		const std::string shapeId = p_reader.require<std::string>("shape");
		const pg::ShapeDefinition *shape = p_shapes.tryGet(shapeId);
		if (shape == nullptr)
		{
			std::string knownIds;
			for (const std::string &id : p_shapes.ids())
			{
				if (!knownIds.empty())
				{
					knownIds += ", ";
				}
				knownIds += id;
			}
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("shape"),
				"unknown shape '" + shapeId + "' (known shapes: " + knownIds + ")");
		}
		return *shape;
	}

	pg::FluidData parseFluid(pg::JsonReader &p_reader, const spk::VoxelShape::TextureSlots &p_textures)
	{
		// A fluid voxel is the authored source block; the registry generates its thinner
		// fill stages (slabs) from this block. The stage count == maxSpread, the material's
		// viscosity (how far it flows). Stages reuse the source's "top" texture cell.
		p_reader.forbidUnknown({"maxSpread"});
		const int maxSpread = p_reader.optional<int>("maxSpread", 8);
		if (maxSpread < 1 || maxSpread > 16)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("maxSpread"), "maxSpread must be between 1 and 16");
		}

		const auto topTexture = p_textures.find("top");
		if (topTexture == p_textures.end())
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.path(),
				"fluid voxels need a 'top' texture slot (reused for the generated fill stages)");
		}
		return {.maxSpread = maxSpread, .falls = true, .texture = topTexture->second};
	}
}

namespace pg
{
	ParsedVoxel parseVoxelDefinition(JsonReader &p_reader, const ShapeCatalog &p_shapes)
	{
		p_reader.forbidUnknown({"version", "traversal", "tags", "transparency", "shape", "textures", "fluid"});

		const int version = p_reader.require<int>("version");
		if (version != 2)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version " + std::to_string(version) + " (expected 2)");
		}

		static const std::map<std::string, VoxelTraversal> traversalValues = {
			{"passable", VoxelTraversal::Passable},
			{"solid", VoxelTraversal::Solid}};

		ParsedVoxel result;
		result.data.traversal = p_reader.requireEnum<VoxelTraversal>("traversal", traversalValues);
		result.data.tags = p_reader.require<std::vector<std::string>>("tags");
		const float transparency = p_reader.optional<float>("transparency", 0.0f);
		if (transparency < 0.0f || transparency > 1.0f)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("transparency"),
				"transparency must be between 0 (opaque) and 1 (invisible)");
		}

		const ShapeDefinition &shape = resolveShape(p_reader, p_shapes);
		JsonReader texturesReader = p_reader.child("textures");
		spk::VoxelShape::TextureSlots textures = readTextures(texturesReader, shape);

		if (p_reader.contains("fluid"))
		{
			JsonReader fluidReader = p_reader.child("fluid");
			result.fluid = parseFluid(fluidReader, textures);
		}

		result.heights = shape.heights;
		result.shape = shape.instantiate(std::move(textures));
		result.shape->setTransparency(transparency);
		return result;
	}
}
