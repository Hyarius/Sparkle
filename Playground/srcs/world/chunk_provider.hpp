#pragma once

namespace spk
{
	class VoxelChunk;
}

namespace pg
{
	class IChunkProvider
	{
	public:
		virtual ~IChunkProvider() = default;
		virtual void fill(spk::VoxelChunk &p_chunk) const = 0;
	};
}
