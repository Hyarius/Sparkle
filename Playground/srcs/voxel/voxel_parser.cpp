#include "voxel/voxel_definition.hpp"

#include "core/json.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <set>
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

	// The state must provide exactly the texture slots its shape's polygons reference: a
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
		// fill stages (slabs) as further states of the same type. The stage count ==
		// maxSpread, the material's viscosity (how far it flows). Stages reuse the
		// source's "top" texture cell.
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

	// One instantiated state: the shared shape bound to this state's textures, sharing
	// the type-level transparency, with the shape definition's walk heights.
	[[nodiscard]] pg::ParsedVoxelState makeState(
		spk::VoxelStateId p_id,
		std::string p_name,
		const pg::ShapeDefinition &p_shape,
		spk::VoxelShape::TextureSlots p_textures,
		float p_transparency)
	{
		pg::ParsedVoxelState state;
		state.id = p_id;
		state.name = std::move(p_name);
		state.heights = p_shape.heights;
		state.shape = p_shape.instantiate(std::move(p_textures));
		state.shape->setTransparency(p_transparency);
		return state;
	}

	// Version 3 "states" array: explicit contiguous state ids from 0, unique non-empty
	// names when provided, one full texture set per state against the shared shape.
	[[nodiscard]] std::vector<pg::ParsedVoxelState> parseStates(
		pg::JsonReader &p_reader,
		const pg::ShapeDefinition &p_shape,
		float p_transparency)
	{
		struct AuthoredState
		{
			std::string name;
			spk::VoxelShape::TextureSlots textures;
			bool present = false;
		};

		std::vector<AuthoredState> authored;
		std::set<std::string> names;
		std::size_t stateCount = 0;
		for (pg::JsonReader stateReader : p_reader.childArray("states"))
		{
			stateReader.forbidUnknown({"id", "name", "textures"});
			const int id = stateReader.require<int>("id");
			if (id < 0 || id > static_cast<int>(std::numeric_limits<std::uint16_t>::max()))
			{
				throw pg::JsonError(stateReader.file(), stateReader.pathFor("id"), "state id is out of range");
			}

			AuthoredState state;
			state.present = true;
			if (stateReader.contains("name"))
			{
				state.name = stateReader.require<std::string>("name");
				if (state.name.empty())
				{
					throw pg::JsonError(stateReader.file(), stateReader.pathFor("name"), "state name must not be empty");
				}
				if (!names.insert(state.name).second)
				{
					throw pg::JsonError(
						stateReader.file(),
						stateReader.pathFor("name"),
						"duplicate state name '" + state.name + "'");
				}
			}
			pg::JsonReader texturesReader = stateReader.child("textures");
			state.textures = readTextures(texturesReader, p_shape);

			// Array order does not define semantic ids: place each state at its
			// explicit id so gaps and duplicates surface as errors.
			if (authored.size() <= static_cast<std::size_t>(id))
			{
				authored.resize(static_cast<std::size_t>(id) + 1);
			}
			if (authored[static_cast<std::size_t>(id)].present)
			{
				throw pg::JsonError(
					stateReader.file(),
					stateReader.pathFor("id"),
					"duplicate state id " + std::to_string(id));
			}
			authored[static_cast<std::size_t>(id)] = std::move(state);
			++stateCount;
		}
		if (stateCount == 0)
		{
			throw pg::JsonError(p_reader.file(), p_reader.pathFor("states"), "at least one state is required");
		}
		for (std::size_t index = 0; index < authored.size(); ++index)
		{
			if (!authored[index].present)
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor("states"),
					"state ids must be contiguous from 0 (missing state " + std::to_string(index) + ")");
			}
		}

		std::vector<pg::ParsedVoxelState> states;
		states.reserve(authored.size());
		for (std::size_t index = 0; index < authored.size(); ++index)
		{
			states.push_back(makeState(
				spk::VoxelStateId{static_cast<std::uint16_t>(index)},
				std::move(authored[index].name),
				p_shape,
				std::move(authored[index].textures),
				p_transparency));
		}
		return states;
	}
}

namespace pg
{
	ParsedVoxel parseVoxelDefinition(JsonReader &p_reader, const ShapeCatalog &p_shapes)
	{
		p_reader.forbidUnknown({"version", "traversal", "tags", "transparency", "shape", "textures", "states", "fluid"});

		const int version = p_reader.require<int>("version");
		if (version != 2 && version != 3)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("version"),
				"unsupported version " + std::to_string(version) + " (expected 2 or 3)");
		}

		static const std::map<std::string, VoxelTraversal> traversalValues = {
			{"passable", VoxelTraversal::Passable},
			{"solid", VoxelTraversal::Solid}};

		// Type-level fields: shared by every state of the voxel.
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

		if (version == 2)
		{
			// A version 2 file is one voxel type with one state: id 0, name "default".
			if (p_reader.contains("states"))
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("states"),
					"'states' requires version 3 (version 2 voxels author one top-level 'textures' block)");
			}
			JsonReader texturesReader = p_reader.child("textures");
			spk::VoxelShape::TextureSlots textures = readTextures(texturesReader, shape);

			if (p_reader.contains("fluid"))
			{
				JsonReader fluidReader = p_reader.child("fluid");
				result.fluid = parseFluid(fluidReader, textures);
			}

			result.states.push_back(makeState(spk::VoxelStateId{}, "default", shape, std::move(textures), transparency));
			return result;
		}

		// Version 3: textures live inside each authored state, never at the top level.
		if (p_reader.contains("textures"))
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("textures"),
				"version 3 voxels author textures per state ('textures' belongs inside 'states')");
		}
		// Fluids keep their version 2 single-state authoring until the dedicated Sparkle
		// fluid migration lands: their extra states are generated, never authored.
		if (p_reader.contains("fluid"))
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("fluid"),
				"'fluid' and authored 'states' cannot coexist (fluid voxels stay on version 2)");
		}
		if (!p_reader.contains("states"))
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("states"), "version 3 voxels require a 'states' array");
		}
		result.states = parseStates(p_reader, shape, transparency);
		return result;
	}
}
