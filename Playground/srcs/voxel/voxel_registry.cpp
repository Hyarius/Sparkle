#include "voxel/voxel_registry.hpp"

#include "core/registry.hpp"
#include "voxel/shape_catalog.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace
{
	// Fluid states expose their rendered fill height as flat navigation heights (the fluid
	// surface, identical in every direction). This never makes a fluid ordinary walking
	// ground - the type is passable - but a later boat-navigation graph can read them.
	[[nodiscard]] pg::CardinalHeightCollection flatHeights(float p_height)
	{
		const pg::CardinalHeightSet set{
			.positiveX = p_height,
			.negativeX = p_height,
			.positiveZ = p_height,
			.negativeZ = p_height,
			.stationary = p_height};
		return {.positiveY = set, .negativeY = set};
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
		const std::vector<std::string> ids = parsed.ids();
		definitions.reserve(ids.size());
		typeToString.reserve(ids.size());

		for (const std::string &id : ids)
		{
			ParsedVoxel &voxel = parsed.getMutable(id);
			VoxelDefinition definition{.id = id, .data = std::move(voxel.data), .light = std::move(voxel.light)};

			if (ParsedFluidVoxel *fluid = std::get_if<ParsedFluidVoxel>(&voxel.rendering); fluid != nullptr)
			{
				// One semantic fluid type whose states Sparkle generates: state 0 is the
				// persistent source, states 1..maxSpread the flow levels. The public
				// semantic id stays the plain string ("water"); state identity is numeric.
				const std::size_t familyIndex = renderRegistry.registerFluid(fluid->description);
				const spk::VoxelFluidFamily &family = renderRegistry.fluidFamily(familyIndex);
				if (family.type.index() != definitions.size())
				{
					throw std::logic_error("voxel registry lost lockstep with the spk render registry");
				}

				definition.typeId = family.type;
				definition.states.reserve(family.levelCount() + 1);
				definition.states.push_back(VoxelStateDefinition{
					.id = family.sourceState,
					.runtimeId = family.sourceRuntime,
					.name = "source",
					.heights = flatHeights(1.0f)});
				for (const spk::VoxelFluidState &level : family.levels)
				{
					definition.states.push_back(VoxelStateDefinition{
						.id = level.state,
						.runtimeId = level.runtime,
						.name = std::to_string(level.level),
						.heights = flatHeights(level.height)});
				}
			}
			else
			{
				ParsedRegularVoxel &regular = std::get<ParsedRegularVoxel>(voxel.rendering);

				// Gather every state shape, then register them transactionally as one type.
				// The parallel vectors keep the state metadata (name + heights) aligned with
				// the shapes handed to the spk registry.
				std::vector<std::unique_ptr<spk::VoxelShape>> stateShapes;
				std::vector<std::string> stateNames;
				std::vector<CardinalHeightCollection> stateHeights;
				stateShapes.reserve(regular.states.size());
				stateNames.reserve(regular.states.size());
				stateHeights.reserve(regular.states.size());
				for (ParsedVoxelState &state : regular.states)
				{
					stateShapes.push_back(std::move(state.shape));
					stateNames.push_back(std::move(state.name));
					stateHeights.push_back(state.heights);
				}

				const std::size_t expectedStates = stateShapes.size();
				const spk::VoxelTypeRegistration registration = renderRegistry.registerType(std::move(stateShapes));
				if (registration.states.size() != expectedStates || registration.type.index() != definitions.size())
				{
					throw std::logic_error("voxel registry lost lockstep with the spk render registry");
				}

				definition.typeId = registration.type;
				definition.states.reserve(registration.states.size());
				for (std::size_t index = 0; index < registration.states.size(); ++index)
				{
					definition.states.push_back(VoxelStateDefinition{
						.id = {static_cast<std::uint16_t>(index)},
						.runtimeId = registration.states[index],
						.name = std::move(stateNames[index]),
						.heights = stateHeights[index]});
				}
			}

			stringToType.emplace(id, definition.typeId);
			definitions.push_back(std::move(definition));
			typeToString.push_back(id);
		}

		_renderRegistry = std::move(renderRegistry);
		_definitions = std::move(definitions);
		_typeToString = std::move(typeToString);
		_stringToType = std::move(stringToType);
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
}
