#include "voxel/voxel_registry.hpp"

#include "core/registry.hpp"

#include <limits>
#include <stdexcept>
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
		std::map<std::string, std::int32_t> stringToNumeric;
		definitions.reserve(ids.size());
		for (const std::string &id : ids)
		{
			ParsedVoxel &voxel = parsed.getMutable(id);
			const std::int32_t numeric = renderRegistry.registerShape(std::move(voxel.shape));
			definitions.push_back(VoxelDefinition{.id = id, .data = std::move(voxel.data), .heights = voxel.heights});
			stringToNumeric.emplace(id, numeric);
		}

		_renderRegistry = std::move(renderRegistry);
		_definitions = std::move(definitions);
		_numericToString = std::move(ids);
		_stringToNumeric = std::move(stringToNumeric);
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
}
