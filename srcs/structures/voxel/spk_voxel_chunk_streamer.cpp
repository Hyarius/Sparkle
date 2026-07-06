#include "structures/voxel/spk_voxel_chunk_streamer.hpp"

#include <stdexcept>

namespace spk
{
	VoxelChunkStreamer::VoxelChunkStreamer(spk::VoxelMap &p_map) :
		_map(&p_map)
	{
	}

	void VoxelChunkStreamer::setViewRange(const spk::Vector3Int &p_viewRange)
	{
		if (p_viewRange.x < 0 || p_viewRange.y < 0 || p_viewRange.z < 0)
		{
			throw std::invalid_argument("VoxelChunkStreamer view range cannot be negative");
		}
		if (_viewRange == p_viewRange)
		{
			return;
		}
		_viewRange = p_viewRange;
		_needsStreaming = true;
	}

	void VoxelChunkStreamer::setOriginPosition(const spk::Vector3Int &p_originChunkPosition)
	{
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

	spk::VoxelMap &VoxelChunkStreamer::map() noexcept
	{
		return *_map;
	}

	const spk::VoxelMap &VoxelChunkStreamer::map() const noexcept
	{
		return *_map;
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
