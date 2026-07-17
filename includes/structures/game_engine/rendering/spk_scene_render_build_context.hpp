#pragma once

#include <cstddef>

#include "structures/game_engine/rendering/spk_scene_render_frame_request.hpp"
#include "structures/graphics/rendering/spk_render_frame_build_context.hpp"

namespace spk
{
	class Camera3D;
	class ComponentRegistry;
	class Profiler;

	struct SceneRenderBuildContext
	{
		spk::RenderFrameBuildContext &frame;
		const spk::SceneRenderFrameRequest &request;
		spk::ComponentRegistry &components;
		spk::Camera3D *mainCamera = nullptr;
		spk::Profiler *profiler = nullptr;
	};
}
