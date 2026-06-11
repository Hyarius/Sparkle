#pragma once

#ifdef _WIN32

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "structures/graphics/opengl/spk_opengl_program.hpp"
#include "structures/graphics/opengl/spk_opengl_texture.hpp"

#include <Windows.h>

#include "structures/math/spk_rect_2d.hpp"
#include "structures/system/device/window/spk_surface_state.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"

namespace spk
{
	class RenderContext
	{
	private:
		struct CompiledProgramEntry
		{
			std::uint64_t version = 0;
			std::unique_ptr<spk::OpenGL::Program> program;
		};

		struct CompiledTextureEntry
		{
			std::uint64_t version = 0;
			std::unique_ptr<spk::OpenGL::Texture> texture;
		};

		static inline thread_local RenderContext* s_current = nullptr;

		std::shared_ptr<SurfaceState> _surfaceState;
		bool _valid = true;

		HWND _windowHandle = nullptr;
		HDC _deviceContext = nullptr;
		HGLRC _renderContext = nullptr;

		std::unordered_map<std::uint64_t, CompiledProgramEntry> _compiledPrograms;
		std::unordered_map<std::uint64_t, CompiledTextureEntry> _compiledTextures;

	protected:
		explicit RenderContext(std::shared_ptr<SurfaceState> p_surfaceState);

	public:
		explicit RenderContext(spk::Frame& p_frame);
		virtual ~RenderContext();

		[[nodiscard]] static RenderContext* current() noexcept;

		[[nodiscard]] spk::OpenGL::Program& compiledProgram(const spk::Program& p_program);
		[[nodiscard]] bool hasCompiledProgram(const spk::Program& p_program) const noexcept;

		[[nodiscard]] spk::OpenGL::Texture& compiledTexture(const spk::Texture& p_texture);
		[[nodiscard]] bool hasCompiledTexture(const spk::Texture& p_texture) const noexcept;

		[[nodiscard]] std::shared_ptr<SurfaceState> surfaceState() const;
		virtual void invalidate();
		[[nodiscard]] virtual bool isValid() const;
		[[nodiscard]] virtual bool supportsOpenGLCommands() const;

		virtual void makeCurrent();
		virtual void present();
		virtual void setVSync(bool p_enabled);
		virtual void notifyResize(const spk::Rect2D& p_rect);
	};
}

#endif
