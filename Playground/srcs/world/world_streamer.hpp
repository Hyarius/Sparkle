#pragma once

#include "world/chunk_coordinates.hpp"

namespace pg
{
	class IChunkProvider;
	class VoxelWorld;

	class WorldStreamer
	{
	private:
		VoxelWorld &_world;
		const IChunkProvider &_provider;
		spk::Vector3Int _radius;

	public:
		WorldStreamer(VoxelWorld &p_world, const IChunkProvider &p_provider, spk::Vector3Int p_radius);
		void update(const spk::Vector3Int &p_focusCell);
	};
}
