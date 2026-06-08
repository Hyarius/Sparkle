#pragma once

#include <cstddef>
#include <string>
#include <mutex>

#include <GL/glew.h>

#include "type/spk_opengl_primitive.hpp"
#include "structures/design_pattern/spk_synchronizable_trait.hpp"

namespace spk
{
	class Program : public spk::SynchronizableTrait
	{
	private:
		std::string _vertexShaderSource;
		std::string _fragmentShaderSource;
		mutable std::mutex _sourceMutex;
		mutable GLuint _id = 0;

	private:
		static GLuint _compileShader(GLenum p_type, const std::string& p_source);
		void _release() const;

	protected:
		void _synchronize() const override;

	public:
		Program() = default;
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

		void activate();
		void deactivate() const;

		void renderRaw(spk::Primitive p_primitive, std::size_t p_firstVertex, std::size_t p_vertexCount);
		void render(spk::Primitive p_primitive, std::size_t p_firstIndex, std::size_t p_indexCount);
	};
}
