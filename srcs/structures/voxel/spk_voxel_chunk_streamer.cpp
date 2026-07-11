#include "structures/voxel/spk_voxel_chunk_streamer.hpp"

#include "structures/voxel/spk_voxel_map.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace spk
{
	VoxelChunkStreamer::VoxelChunkStreamer(spk::VoxelMap &p_map) :
		_map(&p_map),
		_mapLifetime(p_map.lifetimeToken())
	{
	}

	void VoxelChunkStreamer::_validateWindow(
		const spk::Vector3Int &p_originPosition,
		const spk::Vector3Int &p_viewRange)
	{
		if (p_viewRange.x < 0 || p_viewRange.y < 0 || p_viewRange.z < 0)
		{
			throw std::invalid_argument("VoxelChunkStreamer view range cannot be negative");
		}

		const auto validateAxis = [](
			std::int32_t p_origin,
			std::int32_t p_range,
			std::int32_t p_chunkSize,
			const char *p_axis) {
			const std::int64_t minimum = static_cast<std::int64_t>(p_origin) - p_range;
			const std::int64_t maximum = static_cast<std::int64_t>(p_origin) + p_range;
			const std::int64_t minimumChunk = std::numeric_limits<std::int32_t>::min() / p_chunkSize;
			const std::int64_t maximumChunk = std::numeric_limits<std::int32_t>::max() / p_chunkSize;
			if (minimum < minimumChunk || maximum > maximumChunk)
			{
				throw std::out_of_range(
					std::string("VoxelChunkStreamer ") + p_axis +
					" window bound cannot be represented safely in voxel world coordinates");
			}
		};
		validateAxis(p_originPosition.x, p_viewRange.x, spk::VoxelChunk::Size.x, "x");
		validateAxis(p_originPosition.y, p_viewRange.y, spk::VoxelChunk::Size.y, "y");
		validateAxis(p_originPosition.z, p_viewRange.z, spk::VoxelChunk::Size.z, "z");

		std::uint64_t volume = 1;
		for (const std::int32_t range : std::array{p_viewRange.x, p_viewRange.y, p_viewRange.z})
		{
			const std::uint64_t extent = static_cast<std::uint64_t>(range) * 2u + 1u;
			if (extent > MaximumChunkWindowVolume ||
				volume > MaximumChunkWindowVolume / extent)
			{
				throw std::length_error(
					"VoxelChunkStreamer window exceeds the maximum of " +
					std::to_string(MaximumChunkWindowVolume) + " chunks");
			}
			volume *= extent;
		}
	}

	void VoxelChunkStreamer::setViewRange(const spk::Vector3Int &p_viewRange)
	{
		_validateWindow(_originPosition, p_viewRange);
		if (_viewRange == p_viewRange)
		{
			return;
		}
		_viewRange = p_viewRange;
		_needsStreaming = true;
	}

	void VoxelChunkStreamer::setOriginPosition(const spk::Vector3Int &p_originChunkPosition)
	{
		_validateWindow(p_originChunkPosition, _viewRange);
		if (_originPosition == p_originChunkPosition)
		{
			return;
		}
		_originPosition = p_originChunkPosition;
		_needsStreaming = true;
	}

	const spk::Vector3Int &VoxelChunkStreamer::viewRange() const noexcept
	{
		return _viewRange;
	}

	const spk::Vector3Int &VoxelChunkStreamer::originPosition() const noexcept
	{
		return _originPosition;
	}

	bool VoxelChunkStreamer::hasLiveMap() const noexcept
	{
		return _mapLifetime.expired() == false;
	}

	std::weak_ptr<const void> VoxelChunkStreamer::mapLifetime() const noexcept
	{
		return _mapLifetime;
	}

	spk::VoxelMap &VoxelChunkStreamer::map()
	{
		if (hasLiveMap() == false)
		{
			throw std::logic_error("VoxelChunkStreamer map has been destroyed");
		}
		return *_map;
	}

	const spk::VoxelMap &VoxelChunkStreamer::map() const
	{
		if (hasLiveMap() == false)
		{
			throw std::logic_error("VoxelChunkStreamer map has been destroyed");
		}
		return *_map;
	}

	std::size_t VoxelChunkStreamer::windowChunkCount() const noexcept
	{
		return (static_cast<std::size_t>(_viewRange.x) * 2u + 1u) *
			   (static_cast<std::size_t>(_viewRange.y) * 2u + 1u) *
			   (static_cast<std::size_t>(_viewRange.z) * 2u + 1u);
	}

	bool VoxelChunkStreamer::needsStreaming() const noexcept
	{
		return _needsStreaming;
	}

	void VoxelChunkStreamer::markStreamed() noexcept
	{
		_needsStreaming = false;
	}
}
