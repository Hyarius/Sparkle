#include "voxel/voxel_family_definition.hpp"

#include <set>

namespace pg
{
	VoxelFamilyDefinition parseVoxelFamilyDefinition(JsonReader &p_reader, const ShapeCatalog &p_shapes)
	{
		p_reader.forbidUnknown({"version", "baseShape", "variants"});
		const int version = p_reader.require<int>("version");
		if (version != 1)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("version"), "unsupported voxel family version (expected 1)");
		}

		VoxelFamilyDefinition result;
		result.baseShapeId = p_reader.require<std::string>("baseShape");
		const ShapeDefinition *baseShape = p_shapes.tryGet(result.baseShapeId);
		if (baseShape == nullptr)
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("baseShape"), "unknown base shape '" + result.baseShapeId + "'");
		}

		std::set<std::string> names;
		for (JsonReader variantReader : p_reader.childArray("variants"))
		{
			variantReader.forbidUnknown({"name", "shape", "textures"});
			VoxelFamilyVariant variant;
			variant.name = variantReader.require<std::string>("name");
			variant.shapeId = variantReader.require<std::string>("shape");
			if (variant.name.empty() || variant.name == "default" || !names.insert(variant.name).second)
			{
				throw JsonError(variantReader.file(), variantReader.pathFor("name"), "variant names must be unique, non-empty and different from 'default'");
			}
			const ShapeDefinition *shape = p_shapes.tryGet(variant.shapeId);
			if (shape == nullptr)
			{
				throw JsonError(variantReader.file(), variantReader.pathFor("shape"), "unknown variant shape '" + variant.shapeId + "'");
			}

			JsonReader texturesReader = variantReader.child("textures");
			for (const auto &[targetSlot, sourceValue] : texturesReader.value().asObject())
			{
				if (!shape->slots.contains(targetSlot))
				{
					throw JsonError(texturesReader.file(), texturesReader.pathFor(targetSlot), "slot is not used by shape '" + variant.shapeId + "'");
				}
				const std::string sourceSlot = sourceValue.as<std::string>();
				if (!baseShape->slots.contains(sourceSlot))
				{
					throw JsonError(texturesReader.file(), texturesReader.pathFor(targetSlot), "unknown base-material texture slot '" + sourceSlot + "'");
				}
				variant.textureMapping.emplace(targetSlot, sourceSlot);
			}
			if (variant.textureMapping.size() != shape->slots.size())
			{
				throw JsonError(texturesReader.file(), texturesReader.path(), "every texture slot of shape '" + variant.shapeId + "' must be mapped exactly once");
			}
			result.variants.push_back(std::move(variant));
		}
		if (result.variants.empty())
		{
			throw JsonError(p_reader.file(), p_reader.pathFor("variants"), "a voxel family requires at least one variant");
		}
		return result;
	}
}
