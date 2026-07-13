#include "voxel/voxel_definition.hpp"

#include "core/json.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
	std::optional<pg::VoxelLightDefinition> parseLight(pg::JsonReader &reader)
	{
		if (!reader.contains("light")) return std::nullopt;
		pg::JsonReader light = reader.child("light");
		light.forbidUnknown({"type", "color", "power", "reach", "innerHalfAngleDegrees", "outerHalfAngleDegrees"});
		static const std::map<std::string, pg::VoxelLightType> types{{"directional", pg::VoxelLightType::Directional}, {"point", pg::VoxelLightType::Point}, {"spot", pg::VoxelLightType::Spot}};
		pg::VoxelLightDefinition result;
		result.type = light.requireEnum<pg::VoxelLightType>("type", types);
		// Reuse Sparkle's canonical #RRGGBB / #RRGGBBAA color representation.
		result.color = spk::Color(light.require<std::string>("color"));
		result.power = light.require<float>("power");
		result.reach = light.optional<float>("reach", result.reach);
		result.innerHalfAngleDegrees = light.optional<float>("innerHalfAngleDegrees", result.innerHalfAngleDegrees);
		result.outerHalfAngleDegrees = light.optional<float>("outerHalfAngleDegrees", result.outerHalfAngleDegrees);
		if (!std::isfinite(result.power) || result.power < 0.0f || !std::isfinite(result.reach) || result.reach <= 0.0f || !std::isfinite(result.color.r) || !std::isfinite(result.color.g) || !std::isfinite(result.color.b) || result.color.r < 0 || result.color.g < 0 || result.color.b < 0)
			throw pg::JsonError(light.file(), light.path(), "light color must be non-negative and light power/reach must be finite positive values");
		if (result.type == pg::VoxelLightType::Spot && (!std::isfinite(result.innerHalfAngleDegrees) || !std::isfinite(result.outerHalfAngleDegrees) || result.innerHalfAngleDegrees < 0 || result.outerHalfAngleDegrees < result.innerHalfAngleDegrees || result.outerHalfAngleDegrees >= 90.0f))
			throw pg::JsonError(light.file(), light.path(), "spot light angles must satisfy 0 <= inner <= outer < 90");
		return result;
	}
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

	// A fluid voxel authors no geometry and no states: Sparkle generates the source and
	// flow-level shapes from exactly three type-level texture slots. Extra slots are typos
	// worth flagging, missing ones would leave generated geometry untextured.
	spk::VoxelFluidAppearance readFluidAppearance(pg::JsonReader &p_reader, float p_transparency)
	{
		static const std::array<std::string, 3> requiredSlots = {"top", "bottom", "side"};
		for (const std::string &slot : requiredSlots)
		{
			if (!p_reader.contains(slot))
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor(slot),
					"fluid voxels require exactly the 'top', 'bottom' and 'side' texture slots");
			}
		}
		for (const auto &[key, unused] : p_reader.value().asObject())
		{
			(void)unused;
			if (std::find(requiredSlots.begin(), requiredSlots.end(), key) == requiredSlots.end())
			{
				throw pg::JsonError(
					p_reader.file(),
					p_reader.pathFor(key),
					"texture slot '" + key + "' is not used by fluid voxels (only top, bottom and side are)");
			}
		}

		return spk::VoxelFluidAppearance{
			.topTexture = readAtlasCell(p_reader, "top"),
			.bottomTexture = readAtlasCell(p_reader, "bottom"),
			.sideTexture = readAtlasCell(p_reader, "side"),
			.transparency = p_transparency};
	}

	pg::ParsedFluidVoxel parseFluidVoxel(pg::JsonReader &p_reader, pg::VoxelTraversal p_traversal, float p_transparency)
	{
		// Geometry and states are generated by Sparkle, never authored: a fluid is always
		// a full-width slab per fill level, so arbitrary shapes cannot become fluids.
		if (p_reader.contains("shape"))
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("shape"),
				"fluid geometry is engine-generated ('shape' is forbidden on fluid voxels)");
		}
		if (p_reader.contains("states"))
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("states"),
				"'fluid' and authored 'states' cannot coexist (fluid states are generated)");
		}
		if (p_reader.contains("family"))
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("family"),
				"fluid states are engine-generated and cannot use a voxel family");
		}
		if (p_traversal != pg::VoxelTraversal::Passable)
		{
			throw pg::JsonError(
				p_reader.file(),
				p_reader.pathFor("traversal"),
				"fluid voxels must be 'passable' (walking navigation never treats fluid as ground)");
		}

		pg::JsonReader texturesReader = p_reader.child("textures");
		const spk::VoxelFluidAppearance appearance = readFluidAppearance(texturesReader, p_transparency);

		// maxSpread is the material's viscosity: how many cells the fluid flows from a
		// source, which is also its generated fill-level count.
		pg::JsonReader fluidReader = p_reader.child("fluid");
		fluidReader.forbidUnknown({"maxSpread", "falls"});
		const int maxSpread = fluidReader.optional<int>("maxSpread", 8);
		if (maxSpread < 1 || maxSpread > 16)
		{
			throw pg::JsonError(fluidReader.file(), fluidReader.pathFor("maxSpread"), "maxSpread must be between 1 and 16");
		}

		return pg::ParsedFluidVoxel{
			.description = spk::VoxelFluidDescription{
				.levelCount = static_cast<std::size_t>(maxSpread),
				.falls = fluidReader.optional<bool>("falls", true),
				.appearance = appearance}};
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

	// A Playground voxel-family recipe expands one material into named shape states.
	// State 0 remains the family base shape and recipe variants follow in JSON order.
	[[nodiscard]] std::vector<pg::ParsedVoxelState> makeFamilyStates(
		const pg::ShapeDefinition &p_baseShape,
		const spk::VoxelShape::TextureSlots &p_baseTextures,
		const pg::ShapeCatalog &p_shapes,
		const pg::VoxelFamilyDefinition &p_family,
		float p_transparency)
	{
		std::vector<pg::ParsedVoxelState> states;
		states.push_back(makeState(spk::VoxelStateId{}, "default", p_baseShape, p_baseTextures, p_transparency));
		for (const pg::VoxelFamilyVariant &familyVariant : p_family.variants)
		{
			const pg::ShapeDefinition &variant = p_shapes.get(familyVariant.shapeId);
			spk::VoxelShape::TextureSlots textures;
			for (const auto &[targetSlot, sourceSlot] : familyVariant.textureMapping)
			{
				textures.emplace(targetSlot, p_baseTextures.at(sourceSlot));
			}
			states.push_back(makeState(
				spk::VoxelStateId{static_cast<std::uint16_t>(states.size())},
				familyVariant.name,
				variant,
				std::move(textures),
				p_transparency));
		}
		return states;
	}
}

namespace pg
{
	ParsedVoxel parseVoxelDefinition(
		JsonReader &p_reader,
		const ShapeCatalog &p_shapes,
		const VoxelFamilyCatalog &p_families)
	{
		p_reader.forbidUnknown({"version", "traversal", "tags", "transparency", "shape", "family", "textures", "states", "fluid", "light"});

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
		result.light = parseLight(p_reader);
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

		// The presence of a "fluid" block identifies a fluid voxel: its geometry and states
		// are generated by Sparkle from three type-level texture slots.
		if (p_reader.contains("fluid"))
		{
			if (version != 3)
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("fluid"),
					"fluid voxels require version 3");
			}
			result.rendering = parseFluidVoxel(p_reader, result.data.traversal, transparency);
			return result;
		}

		if (p_reader.contains("shape") == p_reader.contains("family"))
		{
			throw JsonError(p_reader.file(), p_reader.path(), "ordinary voxels require exactly one of 'shape' or 'family'");
		}
		const VoxelFamilyDefinition *family = nullptr;
		const ShapeDefinition *shape = nullptr;
		if (p_reader.contains("family"))
		{
			const std::string familyId = p_reader.require<std::string>("family");
			family = p_families.tryGet(familyId);
			if (family == nullptr)
			{
				throw JsonError(p_reader.file(), p_reader.pathFor("family"), "unknown voxel family '" + familyId + "'");
			}
			shape = &p_shapes.get(family->baseShapeId);
		}
		else
		{
			shape = &resolveShape(p_reader, p_shapes);
		}

		if (version == 2)
		{
			// A version 2 file always has the default state and can derive named geometry
			// variants from a cube while keeping one semantic type/material definition.
			if (p_reader.contains("states"))
			{
				throw JsonError(
					p_reader.file(),
					p_reader.pathFor("states"),
					"'states' requires version 3 (version 2 voxels author one top-level 'textures' block)");
			}
			JsonReader texturesReader = p_reader.child("textures");
			spk::VoxelShape::TextureSlots textures = readTextures(texturesReader, *shape);

			ParsedRegularVoxel regular;
			if (family != nullptr)
			{
				regular.states = makeFamilyStates(*shape, textures, p_shapes, *family, transparency);
			}
			else
			{
				regular.states.push_back(makeState(spk::VoxelStateId{}, "default", *shape, std::move(textures), transparency));
			}
			result.rendering = std::move(regular);
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
		if (family != nullptr)
		{
			throw JsonError(
				p_reader.file(),
				p_reader.pathFor("family"),
				"voxel families use version 2 material textures; version 3 authors same-shape states");
		}
		if (!p_reader.contains("states"))
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("states"), "version 3 voxels require a 'states' array");
		}
		result.rendering = ParsedRegularVoxel{.states = parseStates(p_reader, *shape, transparency)};
		return result;
	}
}
