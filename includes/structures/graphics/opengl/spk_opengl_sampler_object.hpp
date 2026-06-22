#pragma once

#include "structures/graphics/opengl/spk_cached_opengl_object_collection.hpp"
#include "structures/graphics/opengl/spk_opengl_uniform_location.hpp"

namespace spk
{
	class RenderContext;
	class SamplerObject;

	namespace OpenGL
	{
		class SamplerObject
		{
		private:
			mutable spk::CachedOpenGLObjectCollection<spk::OpenGL::UniformLocation> _locationCache;

		public:
			void activate(const spk::RenderContext &p_context, const spk::SamplerObject &p_sampler) const;
			void deactivate(const spk::SamplerObject &p_sampler) const;
		};
	}
}
