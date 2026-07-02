#include "voxel/voxel_registry.hpp"

#include <limits>
#include <stdexcept>

namespace pg
{
	void VoxelRegistry::load(const std::filesystem::path &p_directory)
	{
		_definitions.load(p_directory, parseVoxelDefinition);
		_numericToString = _definitions.ids();
		if (_numericToString.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
		{
			throw std::length_error("Voxel registry exceeds int32 numeric id capacity");
		}

		_stringToNumeric.clear();
		_numericDefinitions.clear();
		_numericDefinitions.reserve(_numericToString.size());
		for (std::size_t index = 0; index < _numericToString.size(); ++index)
		{
			_stringToNumeric.emplace(_numericToString[index], static_cast<std::int32_t>(index));
			_numericDefinitions.push_back(&_definitions.get(_numericToString[index]));
		}
	}

	const VoxelDefinition &VoxelRegistry::get(const std::string &p_id) const
	{
		return _definitions.get(p_id);
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
		return _definitions.tryGet(p_id);
	}

	const VoxelDefinition *VoxelRegistry::tryGet(std::int32_t p_id) const noexcept
	{
		if (p_id < 0 || static_cast<std::size_t>(p_id) >= _numericDefinitions.size())
		{
			return nullptr;
		}
		return _numericDefinitions[static_cast<std::size_t>(p_id)];
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
}
