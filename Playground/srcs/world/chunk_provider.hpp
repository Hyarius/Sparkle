#pragma once

namespace pg
{
	class Chunk;

	class IChunkProvider
	{
	public:
		virtual ~IChunkProvider() = default;
		virtual void fill(Chunk &p_chunk) const = 0;
	};
}
