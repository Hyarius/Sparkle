#pragma once

#include <cstddef>

namespace spk
{
	class RenderPassBucketPack;

	struct RenderFrameBuildContext
	{
		spk::RenderPassBucketPack &passes;
		std::size_t frameIndex = 0;
	};
}
