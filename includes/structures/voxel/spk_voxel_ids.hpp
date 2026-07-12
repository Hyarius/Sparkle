#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>

namespace spk
{
	// Identifies one semantic voxel type (ground, bush, water, ...). Type ids are dense:
	// the registry attributes them sequentially from 0.
	struct VoxelTypeId
	{
		std::uint32_t value = 0;

		[[nodiscard]] std::size_t index() const noexcept
		{
			return static_cast<std::size_t>(value);
		}

		auto operator<=>(const VoxelTypeId &) const = default;
	};

	// Identifies one state (variation) of a voxel type: a texture variant, a fluid fill
	// level, a growth or damage stage... State ids of one type are contiguous from 0 and
	// state 0 is the default state, so registries can index states without hashing.
	struct VoxelStateId
	{
		std::uint16_t value = 0;

		[[nodiscard]] std::size_t index() const noexcept
		{
			return static_cast<std::size_t>(value);
		}

		auto operator<=>(const VoxelStateId &) const = default;
	};

	// Dense engine-internal handle stored inside VoxelCell. Each registered state of each
	// type owns exactly one runtime id; the mesher resolves shapes directly through it.
	// Runtime ids depend on registration order: never serialize them as content identity.
	struct VoxelRuntimeId
	{
		static constexpr std::int32_t EmptyValue = -1;

		std::int32_t value = EmptyValue;

		[[nodiscard]] bool isValid() const noexcept
		{
			return value >= 0;
		}

		[[nodiscard]] std::size_t index() const noexcept
		{
			return static_cast<std::size_t>(value);
		}

		auto operator<=>(const VoxelRuntimeId &) const = default;
	};

	// The semantic identity of one registered runtime state.
	struct VoxelStateReference
	{
		spk::VoxelTypeId type;
		spk::VoxelStateId state;

		auto operator<=>(const VoxelStateReference &) const = default;
	};

	inline std::ostream &operator<<(std::ostream &p_stream, const VoxelTypeId &p_id)
	{
		return p_stream << "type:" << p_id.value;
	}

	inline std::ostream &operator<<(std::ostream &p_stream, const VoxelStateId &p_id)
	{
		return p_stream << "state:" << p_id.value;
	}

	inline std::ostream &operator<<(std::ostream &p_stream, const VoxelRuntimeId &p_id)
	{
		return p_stream << "runtime:" << p_id.value;
	}

	inline std::ostream &operator<<(std::ostream &p_stream, const VoxelStateReference &p_reference)
	{
		return p_stream << "{" << p_reference.type << ", " << p_reference.state << "}";
	}
}

template <>
struct std::hash<spk::VoxelTypeId>
{
	std::size_t operator()(const spk::VoxelTypeId &p_id) const noexcept
	{
		return std::hash<std::uint32_t>{}(p_id.value);
	}
};

template <>
struct std::hash<spk::VoxelStateId>
{
	std::size_t operator()(const spk::VoxelStateId &p_id) const noexcept
	{
		return std::hash<std::uint16_t>{}(p_id.value);
	}
};

template <>
struct std::hash<spk::VoxelRuntimeId>
{
	std::size_t operator()(const spk::VoxelRuntimeId &p_id) const noexcept
	{
		return std::hash<std::int32_t>{}(p_id.value);
	}
};
