#pragma once

#include <cstddef>

namespace spk
{
	class RenderPipeline;

	struct RenderFrameBuildContext
	{
		spk::RenderPipeline &passes;
		std::size_t frameIndex = 0;
	};
}
