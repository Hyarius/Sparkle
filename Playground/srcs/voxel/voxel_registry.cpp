#include "voxel/voxel_registry.hpp"

#include "core/registry.hpp"

#include "structures/voxel/spk_cube_voxel_shape.hpp"
#include "structures/voxel/spk_slab_voxel_shape.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace pg
{
	void VoxelRegistry::load(const std::filesystem::path &p_directory)
	{
		// Reuse the generic registry for file discovery, sorted-id ordering and duplicate-id
		// checks, then split each parsed voxel into the render shape and the gameplay catalog.
		Registry<ParsedVoxel> parsed;
		parsed.load(p_directory, parseVoxelDefinition);

		std::vector<std::string> ids = parsed.ids();
		if (ids.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
		{
			throw std::length_error("Voxel registry exceeds int32 numeric id capacity");
		}

		spk::VoxelRegistry renderRegistry;
		std::vector<VoxelDefinition> definitions;
		std::vector<std::string> numericToString;
		std::map<std::string, std::int32_t> stringToNumeric;
		std::vector<FluidFamily> fluids;
		std::unordered_map<std::int32_t, FluidRef> fluidRefs;
		definitions.reserve(ids.size());
		numericToString.reserve(ids.size());

		// Registers one shape + its parallel gameplay definition, keeping the render registry id,
		// the definition table and the id<->name maps in lockstep. Generated fluid stages go
		// through here too, so they get their own ids without disturbing the authored voxels.
		const auto registerVoxel = [&](const std::string &p_id,
									   std::unique_ptr<spk::VoxelShape> p_shape,
									   VoxelData p_data,
									   const CardinalHeightCollection &p_heights) -> std::int32_t {
			const std::int32_t numeric = renderRegistry.registerShape(std::move(p_shape));
			definitions.push_back(VoxelDefinition{.id = p_id, .data = std::move(p_data), .heights = p_heights});
			numericToString.push_back(p_id);
			stringToNumeric.emplace(p_id, numeric);
			return numeric;
		};

		for (const std::string &id : ids)
		{
			ParsedVoxel &voxel = parsed.getMutable(id);
			const std::optional<FluidData> fluid = voxel.fluid;
			const VoxelData tagsSource = voxel.data;				// copied before the move, reused for stage tags
			const float transparency = voxel.shape->transparency(); // reused for the generated stage slabs
			if (fluid.has_value())
			{
				// Every generated fill stage is the same optical medium as its authored
				// source, even when rounding gives their alpha values a small difference.
				voxel.shape->setTransparentOcclusionGroup(id);
			}
			const std::int32_t sourceNumeric = registerVoxel(id, std::move(voxel.shape), std::move(voxel.data), voxel.heights);

			if (fluid.has_value() == false)
			{
				continue;
			}

			// One slab per fill stage: stage k renders at height k / maxSpread, so the fluid is
			// a near-full cube next to the source and a thin film at the edge of its reach. The
			// stages are passable and carry the source's tags (water, ...) for gameplay queries.
			const std::size_t familyIndex = fluids.size();
			FluidFamily family;
			family.sourceId = sourceNumeric;
			family.maxSpread = fluid->maxSpread;
			family.falls = fluid->falls;
			family.stageIds.reserve(static_cast<std::size_t>(fluid->maxSpread));
			for (int stage = 1; stage <= fluid->maxSpread; ++stage)
			{
				const float fill = static_cast<float>(stage) / static_cast<float>(fluid->maxSpread);
				VoxelData stageData;
				stageData.traversal = VoxelTraversal::Passable;
				stageData.tags = tagsSource.tags;
				// The full stage is a cube so all six faces participate in boundary occlusion.
				std::unique_ptr<spk::VoxelShape> stageShape;
				if (stage == fluid->maxSpread)
				{
					stageShape = std::make_unique<spk::CubeVoxelShape>(fluid->texture);
				}
				else
				{
					stageShape = std::make_unique<spk::SlabVoxelShape>(fluid->texture, fill);
				}
				// Stages share the source's transparency so a water body culls its internal
				// faces and renders as one continuous translucent volume.
				stageShape->setTransparency(transparency);
				stageShape->setTransparentOcclusionGroup(id);
				const std::int32_t stageNumeric = registerVoxel(
					id + "#" + std::to_string(stage),
					std::move(stageShape),
					std::move(stageData),
					CardinalHeightCollection{});
				family.stageIds.push_back(stageNumeric);
				fluidRefs.emplace(stageNumeric, FluidRef{.family = familyIndex, .stage = stage, .source = false});
			}
			fluidRefs.emplace(sourceNumeric, FluidRef{.family = familyIndex, .stage = fluid->maxSpread, .source = true});
			fluids.push_back(std::move(family));
		}

		_renderRegistry = std::move(renderRegistry);
		_definitions = std::move(definitions);
		_numericToString = std::move(numericToString);
		_stringToNumeric = std::move(stringToNumeric);
		_fluids = std::move(fluids);
		_fluidRefs = std::move(fluidRefs);
	}

	const VoxelDefinition &VoxelRegistry::get(const std::string &p_id) const
	{
		return get(numericId(p_id));
	}

	const VoxelDefinition &VoxelRegistry::get(std::int32_t p_id) const
	{
		const VoxelDefinition *definition = tryGet(p_id);
		if (definition == nullptr)
		{
			throw std::out_of_range("unknown numeric voxel registry id");
		}
		return *definition;
	}

	const VoxelDefinition *VoxelRegistry::tryGet(const std::string &p_id) const noexcept
	{
		const auto iterator = _stringToNumeric.find(p_id);
		return iterator == _stringToNumeric.end() ? nullptr : tryGet(iterator->second);
	}

	const VoxelDefinition *VoxelRegistry::tryGet(std::int32_t p_id) const noexcept
	{
		if (p_id < 0 || static_cast<std::size_t>(p_id) >= _definitions.size())
		{
			return nullptr;
		}
		return &_definitions[static_cast<std::size_t>(p_id)];
	}

	std::int32_t VoxelRegistry::numericId(const std::string &p_id) const
	{
		const auto iterator = _stringToNumeric.find(p_id);
		if (iterator == _stringToNumeric.end())
		{
			throw std::out_of_range("unknown voxel registry id '" + p_id + "'");
		}
		return iterator->second;
	}

	const std::string &VoxelRegistry::stringId(std::int32_t p_id) const
	{
		if (p_id < 0 || static_cast<std::size_t>(p_id) >= _numericToString.size())
		{
			throw std::out_of_range("unknown numeric voxel registry id");
		}
		return _numericToString[static_cast<std::size_t>(p_id)];
	}

	const std::vector<std::string> &VoxelRegistry::ids() const noexcept
	{
		return _numericToString;
	}

	std::size_t VoxelRegistry::size() const noexcept
	{
		return _numericToString.size();
	}

	const spk::VoxelRegistry &VoxelRegistry::renderRegistry() const noexcept
	{
		return _renderRegistry;
	}

	const std::vector<FluidFamily> &VoxelRegistry::fluids() const noexcept
	{
		return _fluids;
	}

	const FluidRef *VoxelRegistry::tryFluidRef(std::int32_t p_id) const noexcept
	{
		const auto iterator = _fluidRefs.find(p_id);
		return iterator == _fluidRefs.end() ? nullptr : &iterator->second;
	}
}
