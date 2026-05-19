#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <string>
#include <mutex>

#include <GL/glew.h>

#include "spk_synchronizable_trait.hpp"

namespace spk
{
	class Program : public spk::SynchronizableTrait
	{
	private:
		std::string _vertexShaderSource;
		std::string _fragmentShaderSource;
		mutable std::mutex _sourceMutex;
		GLuint _id = 0;

	private:
		static GLuint _compileShader(GLenum p_type, const std::string& p_source);
		void _release();

	protected:
		void _synchronize() override;

	public:
		Program(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);
		~Program();

		Program(const Program&) = delete;
		Program& operator=(const Program&) = delete;
		Program(Program&&) noexcept = delete;
		Program& operator=(Program&&) noexcept = delete;

		[[nodiscard]] GLuint id();
		[[nodiscard]] bool isLinked() const noexcept;

		void setSources(std::string p_vertexShaderSource, std::string p_fragmentShaderSource);
		[[nodiscard]] std::string vertexShaderSource() const;
		[[nodiscard]] std::string fragmentShaderSource() const;

		void bind();
		void unbind() const;
	};
}

#endif
