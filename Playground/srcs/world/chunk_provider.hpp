#pragma once

namespace pg
{
	class Chunk;
	struct MapDefinition;

	class IChunkProvider
	{
	public:
		virtual ~IChunkProvider() = default;
		virtual void fill(Chunk &p_chunk) const = 0;
	};

	class MapChunkProvider final : public IChunkProvider
	{
	private:
		const MapDefinition &_map;

	public:
		explicit MapChunkProvider(const MapDefinition &p_map);
		void fill(Chunk &p_chunk) const override;
	};
}
