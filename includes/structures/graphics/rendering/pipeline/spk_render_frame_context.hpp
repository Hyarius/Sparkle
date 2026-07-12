#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "structures/graphics/geometry/spk_color.hpp"
#include "structures/graphics/rendering/pipeline/spk_render_phase.hpp"
#include "structures/graphics/rendering/state/spk_viewport.hpp"

namespace spk
{
	class Camera3D;
	class ComponentRegistry;
	class FrameBufferObject;
	class Profiler;
	struct RenderPassDescription;

	struct RenderTargetReference
	{
		const spk::FrameBufferObject *frameBuffer = nullptr;
		spk::Viewport viewport;
	};

	struct RenderPassClear
	{
		std::optional<spk::Color> color;
		std::optional<float> depth;
		std::optional<std::uint32_t> stencil;
	};

	struct RenderFrameRequest
	{
		RenderTargetReference mainTarget;
		RenderPassClear mainClear;
	};

	struct RenderFrameContext
	{
		const RenderFrameRequest &request;
		spk::ComponentRegistry &components;
		spk::Camera3D *mainCamera = nullptr;
		spk::Profiler *profiler = nullptr;
		std::size_t frameIndex = 0;
	};

	class RenderPhaseFeatureData
	{
	public:
		virtual ~RenderPhaseFeatureData() = default;
	};

	struct RenderPhaseContext
	{
		const RenderFrameContext &frame;
		const RenderPassDescription &pass;
		RenderPhase phase;
		std::size_t passInstance = 0;
		std::shared_ptr<const spk::RenderPhaseFeatureData> featureData;

		template <typename TData>
			requires std::derived_from<TData, spk::RenderPhaseFeatureData>
		[[nodiscard]] const TData *feature() const noexcept
		{
			return dynamic_cast<const TData *>(featureData.get());
		}
	};
}
