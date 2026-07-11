#pragma once

#include "structures/game_engine/spk_component.hpp"
#include "structures/math/spk_vector3.hpp"

#include <cstddef>
#include <memory>

namespace spk
{
	class VoxelMap;

	// Describes the window of chunks an observer wants active in a VoxelMap: an origin
	// (in chunk coordinates) and an inclusive view range around it, per axis. Attach it to
	// any entity (a player controller typically re-emits setOriginPosition whenever it
	// crosses a chunk boundary); the VoxelChunkStreamerLogic reconciles the map's chunks
	// with the requested window before the render logic runs.
	class VoxelChunkStreamer final : public spk::Component
	{
	public:
		// A single map may request at most this many distinct chunks per update. This
		// keeps accepted windows synchronously bounded while still covering the
		// Playground's normal 5 x 3 x 5 window comfortably.
		static constexpr std::size_t MaximumChunkWindowVolume = std::numeric_limits<std::size_t>::max();//4096;

	private:
		spk::VoxelMap *_map = nullptr;
		std::weak_ptr<const void> _mapLifetime;
		spk::Vector3Int _viewRange{0, 0, 0};
		spk::Vector3Int _originPosition{0, 0, 0};
		bool _needsStreaming = true;

		static void _validateWindow(
			const spk::Vector3Int &p_originPosition,
			const spk::Vector3Int &p_viewRange);

	public:
		explicit VoxelChunkStreamer(spk::VoxelMap &p_map);

		// Inclusive radius in chunks: the window spans [origin - range, origin + range]
		// on every axis. Negative, unrepresentable, and excessively large windows are
		// rejected before streamer state is changed.
		void setViewRange(const spk::Vector3Int &p_viewRange);
		// Chunk coordinates of the window's center. The complete window must remain
		// representable by Vector3Int.
		void setOriginPosition(const spk::Vector3Int &p_originChunkPosition);

		[[nodiscard]] const spk::Vector3Int &viewRange() const noexcept;
		[[nodiscard]] const spk::Vector3Int &originPosition() const noexcept;

		// VoxelMap is borrowed rather than owned. Access after the map has been
		// destroyed fails deterministically instead of dereferencing a dangling pointer.
		[[nodiscard]] bool hasLiveMap() const noexcept;
		[[nodiscard]] std::weak_ptr<const void> mapLifetime() const noexcept;
		[[nodiscard]] spk::VoxelMap &map();
		[[nodiscard]] const spk::VoxelMap &map() const;
		[[nodiscard]] std::size_t windowChunkCount() const noexcept;

		[[nodiscard]] bool needsStreaming() const noexcept;
		void markStreamed() noexcept;
	};
}
