#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "opengl/spk_opengl_shader_storage_buffer_object.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"

namespace spk::OpenGL
{
	class GPUDataBufferCenter
	{
	public:
		static constexpr std::string_view ViewportBlockName = "ViewportData";

	private:
		using Buffer = std::variant<
			std::shared_ptr<spk::OpenGL::UniformBufferObject>,
			std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject>>;

		static inline std::unordered_map<std::string, Buffer> _buffers;

	public:
		static void addUBO(
			std::string_view p_name,
			std::shared_ptr<spk::OpenGL::UniformBufferObject> p_buffer);

		static void addSSBO(
			std::string_view p_name,
			std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject> p_buffer);

		static spk::OpenGL::UniformBufferObject& getUBO(std::string_view p_name);

		static spk::OpenGL::ShaderStorageBufferObject& getSSBO(std::string_view p_name);

		[[nodiscard]] static bool contains(std::string_view p_name);

		static void remove(std::string_view p_name);
	};
}

#endif
