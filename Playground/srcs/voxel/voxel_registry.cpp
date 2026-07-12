#include "voxel/voxel_registry.hpp"

#include "core/registry.hpp"
#include "voxel/shape_catalog.hpp"

#include "structures/voxel/spk_data_voxel_shape.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
	// Fill-stage geometry for fluids: a slab of the given height (the full source cube is
	// the height-1 case), every polygon on a single "fluid" slot textured with the source's
	// top cell. Mid-height tops are inner faces, so a buried stage inside a fluid body
	// culls its surface while an exposed one still renders it.
	[[nodiscard]] spk::VoxelShapeDescription fluidStageDescription(float p_height)
	{
		const std::vector<spk::Vector2> rectangleUVs = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
		const std::vector<spk::Vector2> sideUVs = {{0, 1}, {1, 1}, {1, 1.0f - p_height}, {0, 1.0f - p_height}};

		spk::VoxelShapeDescription description;
		description.polygons = {
			{"fluid", {{0, p_height, 1}, {1, p_height, 1}, {1, p_height, 0}, {0, p_height, 0}}, rectangleUVs},
			{"fluid", {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, rectangleUVs},
			{"fluid", {{1, 0, 1}, {1, 0, 0}, {1, p_height, 0}, {1, p_height, 1}}, sideUVs},
			{"fluid", {{0, 0, 0}, {0, 0, 1}, {0, p_height, 1}, {0, p_height, 0}}, sideUVs},
			{"fluid", {{0, 0, 1}, {1, 0, 1}, {1, p_height, 1}, {0, p_height, 1}}, sideUVs},
			{"fluid", {{1, 0, 0}, {0, 0, 0}, {0, p_height, 0}, {1, p_height, 0}}, sideUVs},
		};
		return description;
	}

	[[nodiscard]] std::unique_ptr<spk::VoxelShape> makeFluidStageShape(float p_height, const spk::AtlasCell &p_texture)
	{
		return std::make_unique<spk::DataVoxelShape>(
			spk::VoxelShape::TextureSlots{{"fluid", p_texture}},
			fluidStageDescription(p_height));
	}
}

namespace pg
{
	void VoxelRegistry::load(const ShapeCatalog &p_shapes, const std::filesystem::path &p_voxelsDirectory)
	{
		// Reuse the generic registry for file discovery, sorted-id ordering and duplicate-id
		// checks, then register each parsed voxel as ONE spk voxel type whose states are its
		// authored (or, for fluids, generated) render shapes.
		Registry<ParsedVoxel> parsed;
		spk::loadJsonDirectory(parsed, p_voxelsDirectory, [&p_shapes](std::string_view p_id, JsonReader &p_reader) {
			ParsedVoxel voxel = parseVoxelDefinition(p_reader, p_shapes);
			voxel.id = p_id;
			return voxel;
		});

		spk::VoxelRegistry renderRegistry;
		std::vector<VoxelDefinition> definitions;
		std::vector<std::string> typeToString;
		std::map<std::string, spk::VoxelTypeId> stringToType;
		std::vector<FluidFamily> fluids;
		std::unordered_map<spk::VoxelRuntimeId, FluidRef> fluidRefs;
		const std::vector<std::string> ids = parsed.ids();
		definitions.reserve(ids.size());
		typeToString.reserve(ids.size());

		for (const std::string &id : ids)
		{
			ParsedVoxel &voxel = parsed.getMutable(id);
			const std::optional<FluidData> fluid = voxel.fluid;

			// Gather every state shape, then register them transactionally as one type.
			// The parallel vectors keep the state metadata (name + heights) aligned with
			// the shapes handed to the spk registry.
			std::vector<std::unique_ptr<spk::VoxelShape>> stateShapes;
			std::vector<std::string> stateNames;
			std::vector<CardinalHeightCollection> stateHeights;
			stateShapes.reserve(voxel.states.size());
			stateNames.reserve(voxel.states.size());
			stateHeights.reserve(voxel.states.size());
			for (ParsedVoxelState &state : voxel.states)
			{
				stateShapes.push_back(std::move(state.shape));
				stateNames.push_back(std::move(state.name));
				stateHeights.push_back(state.heights);
			}

			if (fluid.has_value())
			{
				// One semantic fluid type: state 0 is the authored source, states
				// 1..maxSpread the generated fill stages. Every stage is the same optical
				// medium as its source (shared transparency + occlusion group), so a fluid
				// body culls its internal faces and renders as one translucent volume.
				const float transparency = stateShapes.front()->transparency();
				stateShapes.front()->setTransparentOcclusionGroup(id);
				stateNames.front() = "source";
				for (int stage = 1; stage <= fluid->maxSpread; ++stage)
				{
					const float fill = static_cast<float>(stage) / static_cast<float>(fluid->maxSpread);
					std::unique_ptr<spk::VoxelShape> stageShape = makeFluidStageShape(fill, fluid->texture);
					stageShape->setTransparency(transparency);
					stageShape->setTransparentOcclusionGroup(id);
					stateShapes.push_back(std::move(stageShape));
					stateNames.push_back(std::to_string(stage));
					stateHeights.push_back(CardinalHeightCollection{});
				}
			}

			const std::size_t expectedStates = stateShapes.size();
			const spk::VoxelTypeRegistration registration = renderRegistry.registerType(std::move(stateShapes));
			if (registration.states.size() != expectedStates || registration.type.index() != definitions.size())
			{
				throw std::logic_error("voxel registry lost lockstep with the spk render registry");
			}

			VoxelDefinition definition{.id = id, .typeId = registration.type, .data = std::move(voxel.data)};
			definition.states.reserve(registration.states.size());
			for (std::size_t index = 0; index < registration.states.size(); ++index)
			{
				definition.states.push_back(VoxelStateDefinition{
					.id = {static_cast<std::uint16_t>(index)},
					.runtimeId = registration.states[index],
					.name = std::move(stateNames[index]),
					.heights = stateHeights[index]});
			}

			if (fluid.has_value())
			{
				const std::size_t familyIndex = fluids.size();
				FluidFamily family;
				family.type = registration.type;
				family.maxSpread = fluid->maxSpread;
				family.falls = fluid->falls;
				family.source = FluidStateRef{
					.type = registration.type,
					.state = {},
					.runtime = registration.states.front(),
					.level = static_cast<std::size_t>(fluid->maxSpread),
					.source = true};
				family.levels.reserve(static_cast<std::size_t>(fluid->maxSpread));
				for (int stage = 1; stage <= fluid->maxSpread; ++stage)
				{
					const spk::VoxelRuntimeId runtime = registration.states[static_cast<std::size_t>(stage)];
					family.levels.push_back(FluidStateRef{
						.type = registration.type,
						.state = {static_cast<std::uint16_t>(stage)},
						.runtime = runtime,
						.level = static_cast<std::size_t>(stage),
						.source = false});
					fluidRefs.emplace(runtime, FluidRef{.family = familyIndex, .stage = stage, .source = false});
				}
				fluidRefs.emplace(
					family.source.runtime,
					FluidRef{.family = familyIndex, .stage = fluid->maxSpread, .source = true});
				fluids.push_back(std::move(family));
			}

			definitions.push_back(std::move(definition));
			typeToString.push_back(id);
			stringToType.emplace(id, registration.type);
		}

		_renderRegistry = std::move(renderRegistry);
		_definitions = std::move(definitions);
		_typeToString = std::move(typeToString);
		_stringToType = std::move(stringToType);
		_fluids = std::move(fluids);
		_fluidRefs = std::move(fluidRefs);
	}

	const VoxelDefinition &VoxelRegistry::get(const std::string &p_id) const
	{
		return get(typeId(p_id));
	}

	const VoxelDefinition &VoxelRegistry::get(spk::VoxelTypeId p_type) const
	{
		const VoxelDefinition *definition = tryGet(p_type);
		if (definition == nullptr)
		{
			throw std::out_of_range("unknown voxel type id " + std::to_string(p_type.value));
		}
		return *definition;
	}

	const VoxelDefinition *VoxelRegistry::tryGet(const std::string &p_id) const noexcept
	{
		const auto iterator = _stringToType.find(p_id);
		return iterator == _stringToType.end() ? nullptr : tryGet(iterator->second);
	}

	const VoxelDefinition *VoxelRegistry::tryGet(spk::VoxelTypeId p_type) const noexcept
	{
		if (p_type.index() >= _definitions.size())
		{
			return nullptr;
		}
		return &_definitions[p_type.index()];
	}

	const VoxelDefinition &VoxelRegistry::definition(spk::VoxelRuntimeId p_runtime) const
	{
		const VoxelDefinition *result = tryDefinition(p_runtime);
		if (result == nullptr)
		{
			throw std::out_of_range("unknown voxel runtime id " + std::to_string(p_runtime.value));
		}
		return *result;
	}

	const VoxelDefinition *VoxelRegistry::tryDefinition(spk::VoxelRuntimeId p_runtime) const noexcept
	{
		const spk::VoxelStateReference *reference = _renderRegistry.tryStateReference(p_runtime);
		return reference == nullptr ? nullptr : tryGet(reference->type);
	}

	const VoxelStateDefinition &VoxelRegistry::state(spk::VoxelRuntimeId p_runtime) const
	{
		const VoxelStateDefinition *result = tryState(p_runtime);
		if (result == nullptr)
		{
			throw std::out_of_range("unknown voxel runtime id " + std::to_string(p_runtime.value));
		}
		return *result;
	}

	const VoxelStateDefinition *VoxelRegistry::tryState(spk::VoxelRuntimeId p_runtime) const noexcept
	{
		const spk::VoxelStateReference *reference = _renderRegistry.tryStateReference(p_runtime);
		if (reference == nullptr)
		{
			return nullptr;
		}
		const VoxelDefinition *definition = tryGet(reference->type);
		return definition == nullptr ? nullptr : definition->tryState(reference->state);
	}

	spk::VoxelTypeId VoxelRegistry::typeId(const std::string &p_id) const
	{
		const auto iterator = _stringToType.find(p_id);
		if (iterator == _stringToType.end())
		{
			throw std::out_of_range("unknown voxel registry id '" + p_id + "'");
		}
		return iterator->second;
	}

	spk::VoxelRuntimeId VoxelRegistry::runtimeId(const std::string &p_id, spk::VoxelStateId p_state) const
	{
		return runtimeId(typeId(p_id), p_state);
	}

	spk::VoxelRuntimeId VoxelRegistry::runtimeId(spk::VoxelTypeId p_type, spk::VoxelStateId p_state) const
	{
		return _renderRegistry.runtimeId(p_type, p_state);
	}

	spk::VoxelRuntimeId VoxelRegistry::numericId(const std::string &p_id) const
	{
		return runtimeId(p_id, spk::VoxelStateId{});
	}

	const std::string &VoxelRegistry::stringId(spk::VoxelTypeId p_type) const
	{
		if (p_type.index() >= _typeToString.size())
		{
			throw std::out_of_range("unknown voxel type id " + std::to_string(p_type.value));
		}
		return _typeToString[p_type.index()];
	}

	std::string VoxelRegistry::debugName(spk::VoxelRuntimeId p_runtime) const
	{
		const VoxelDefinition &owner = definition(p_runtime);
		const VoxelStateDefinition &stateDefinition = state(p_runtime);
		if (stateDefinition.id == spk::VoxelStateId{} && stateDefinition.name == "default")
		{
			return owner.id;
		}
		return owner.id + "@" +
			   (stateDefinition.name.empty() ? std::to_string(stateDefinition.id.value) : stateDefinition.name);
	}

	const std::vector<std::string> &VoxelRegistry::ids() const noexcept
	{
		return _typeToString;
	}

	std::size_t VoxelRegistry::typeCount() const noexcept
	{
		return _definitions.size();
	}

	std::size_t VoxelRegistry::runtimeStateCount() const noexcept
	{
		return _renderRegistry.runtimeStateCount();
	}

	const spk::VoxelRegistry &VoxelRegistry::renderRegistry() const noexcept
	{
		return _renderRegistry;
	}

	const std::vector<FluidFamily> &VoxelRegistry::fluids() const noexcept
	{
		return _fluids;
	}

	const FluidRef *VoxelRegistry::tryFluidRef(spk::VoxelRuntimeId p_runtime) const noexcept
	{
		const auto iterator = _fluidRefs.find(p_runtime);
		return iterator == _fluidRefs.end() ? nullptr : &iterator->second;
	}
}
