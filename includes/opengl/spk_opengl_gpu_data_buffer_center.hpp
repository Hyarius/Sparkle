#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "opengl/spk_opengl_shader_storage_buffer_object.hpp"
#include "opengl/spk_opengl_uniform_buffer_object.hpp"

namespace spk
{
	class GPUDataBufferCenter
	{
	public:
		static constexpr std::string_view ViewportBlockName = "ViewportData";

	private:
		using Buffer = std::variant<
			std::shared_ptr<spk::UniformBufferObject>,
			std::shared_ptr<spk::ShaderStorageBufferObject>>;

		static inline std::unordered_map<std::string, Buffer> _buffers;

	public:
		static void addUBO(
			std::string_view p_name,
			std::shared_ptr<spk::UniformBufferObject> p_buffer);

		static void addSSBO(
			std::string_view p_name,
			std::shared_ptr<spk::ShaderStorageBufferObject> p_buffer);

		static spk::UniformBufferObject& getUBO(std::string_view p_name);

		static spk::ShaderStorageBufferObject& getSSBO(std::string_view p_name);

		[[nodiscard]] static bool contains(std::string_view p_name);

		static void remove(std::string_view p_name);
	};
}
