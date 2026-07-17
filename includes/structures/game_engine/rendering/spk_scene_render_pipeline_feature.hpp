#pragma once

#include "structures/game_engine/rendering/spk_scene_render_build_context.hpp"

namespace spk
{
	struct SceneRenderBuildContext;
	class RenderPipeline;

	class ISceneRenderPipelineFeature
	{
	public:
		virtual ~ISceneRenderPipelineFeature() = default;
		virtual void prepareFrame(const spk::SceneRenderBuildContext &p_context)
		{
			(void)p_context;
		}
		virtual void declarePasses(const spk::SceneRenderBuildContext &p_context, spk::RenderPipeline &p_passes)
		{
			(void)p_context;
			(void)p_passes;
		}
		virtual void contribute(const spk::SceneRenderBuildContext &p_context)
		{
			(void)p_context;
		}
	};
}
