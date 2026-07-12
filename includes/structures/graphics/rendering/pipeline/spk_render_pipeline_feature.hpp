#pragma once

#include "structures/graphics/rendering/pipeline/spk_render_frame_context.hpp"

namespace spk
{
	class RenderPass;
	class RenderPipelineBuilder;

	class IRenderPipelineFeature
	{
	public:
		virtual ~IRenderPipelineFeature() = default;

		virtual void prepareFrame(const RenderFrameContext &p_context)
		{
			(void)p_context;
		}
		virtual void appendPreMainPasses(RenderPipelineBuilder &p_builder, const RenderFrameContext &p_context)
		{
			(void)p_builder;
			(void)p_context;
		}
		virtual void appendMainPassSetup(RenderPass &p_mainPass, const RenderFrameContext &p_context)
		{
			(void)p_mainPass;
			(void)p_context;
		}
		virtual void appendPostMainPasses(RenderPipelineBuilder &p_builder, const RenderFrameContext &p_context)
		{
			(void)p_builder;
			(void)p_context;
		}
	};
}
