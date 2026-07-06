#pragma once

#include "world/chunk_coordinates.hpp"

#include <cstddef>

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
		std::size_t _loadBudget = 4;
		std::size_t _pendingLoadCount = 0;

	public:
		WorldStreamer(
			VoxelWorld &p_world,
			const IChunkProvider &p_provider,
			spk::Vector3Int p_radius,
			std::size_t p_loadBudget = 4);
		void update(const spk::Vector3Int &p_focusCell);
		[[nodiscard]] std::size_t pendingLoadCount() const noexcept;
	};
}
