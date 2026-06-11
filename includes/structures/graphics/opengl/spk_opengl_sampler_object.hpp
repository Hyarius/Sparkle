#pragma once

#include <cstdint>

#include <GL/glew.h>

namespace spk
{
	class RenderContext;
	class SamplerObject;

	namespace OpenGL
	{
		class SamplerObject
		{
		private:
			GLint _uniformDestination = -1;
			std::uint64_t _uniformContextId = 0;
			GLuint _uniformProgramId = 0;

		public:
			void invalidateUniformLocation() noexcept;
			void activate(const spk::RenderContext& p_context, const spk::SamplerObject& p_sampler);
			void deactivate(const spk::SamplerObject& p_sampler);
		};
	}
}
