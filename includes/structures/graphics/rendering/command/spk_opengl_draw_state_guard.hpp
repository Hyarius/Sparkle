#pragma once

#include <GL/glew.h>

namespace spk
{
	// Snapshots the fixed-function draw state a mesh render command touches (culling,
	// depth, blending) and restores it on scope exit, so commands can freely reconfigure
	// the pipeline without leaking state into the next command.
	class OpenGLDrawStateGuard
	{
	private:
		static void _setEnabled(GLenum p_capability, bool p_enabled)
		{
			if (p_enabled)
			{
				glEnable(p_capability);
			}
			else
			{
				glDisable(p_capability);
			}
		}

		const bool _cullEnabled = glIsEnabled(GL_CULL_FACE) == GL_TRUE;
		const bool _depthEnabled = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
		const bool _blendEnabled = glIsEnabled(GL_BLEND) == GL_TRUE;
		GLint _cullFaceMode = GL_BACK;
		GLint _frontFace = GL_CCW;
		GLint _depthFunction = GL_LESS;
		GLboolean _depthWriteMask = GL_TRUE;
		GLint _blendSourceRGB = GL_ONE;
		GLint _blendDestinationRGB = GL_ZERO;
		GLint _blendSourceAlpha = GL_ONE;
		GLint _blendDestinationAlpha = GL_ZERO;

	public:
		OpenGLDrawStateGuard()
		{
			glGetIntegerv(GL_CULL_FACE_MODE, &_cullFaceMode);
			glGetIntegerv(GL_FRONT_FACE, &_frontFace);
			glGetIntegerv(GL_DEPTH_FUNC, &_depthFunction);
			glGetBooleanv(GL_DEPTH_WRITEMASK, &_depthWriteMask);
			glGetIntegerv(GL_BLEND_SRC_RGB, &_blendSourceRGB);
			glGetIntegerv(GL_BLEND_DST_RGB, &_blendDestinationRGB);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &_blendSourceAlpha);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &_blendDestinationAlpha);
		}

		~OpenGLDrawStateGuard()
		{
			glCullFace(static_cast<GLenum>(_cullFaceMode));
			glFrontFace(static_cast<GLenum>(_frontFace));
			glDepthFunc(static_cast<GLenum>(_depthFunction));
			glDepthMask(_depthWriteMask);
			glBlendFuncSeparate(
				static_cast<GLenum>(_blendSourceRGB),
				static_cast<GLenum>(_blendDestinationRGB),
				static_cast<GLenum>(_blendSourceAlpha),
				static_cast<GLenum>(_blendDestinationAlpha));
			_setEnabled(GL_CULL_FACE, _cullEnabled);
			_setEnabled(GL_DEPTH_TEST, _depthEnabled);
			_setEnabled(GL_BLEND, _blendEnabled);
		}

		OpenGLDrawStateGuard(const OpenGLDrawStateGuard &) = delete;
		OpenGLDrawStateGuard &operator=(const OpenGLDrawStateGuard &) = delete;
	};
}
