#include "opengl/spk_opengl_gpu_data_buffer_center.hpp"

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <stdexcept>
#include <utility>

namespace spk::OpenGL
{
	void GPUDataBufferCenter::addUBO(
		std::string_view p_name,
		std::shared_ptr<spk::OpenGL::UniformBufferObject> p_buffer)
	{
		if (p_buffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::addUBO received a null buffer");
		}

		_buffers[std::string(p_name)] = std::move(p_buffer);
	}

	void GPUDataBufferCenter::addSSBO(
		std::string_view p_name,
		std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject> p_buffer)
	{
		if (p_buffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::addSSBO received a null buffer");
		}

		_buffers[std::string(p_name)] = std::move(p_buffer);
	}

	spk::OpenGL::UniformBufferObject& GPUDataBufferCenter::getUBO(std::string_view p_name)
	{
		const auto it = _buffers.find(std::string(p_name));
		if (it == _buffers.end())
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::getUBO could not find the requested buffer");
		}

		auto* buffer = std::get_if<std::shared_ptr<spk::OpenGL::UniformBufferObject>>(&it->second);
		if (buffer == nullptr || *buffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::getUBO found a buffer with a different type");
		}

		return **buffer;
	}

	spk::OpenGL::ShaderStorageBufferObject& GPUDataBufferCenter::getSSBO(std::string_view p_name)
	{
		const auto it = _buffers.find(std::string(p_name));
		if (it == _buffers.end())
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::getSSBO could not find the requested buffer");
		}

		auto* buffer = std::get_if<std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject>>(&it->second);
		if (buffer == nullptr || *buffer == nullptr)
		{
			throw std::runtime_error("spk::OpenGL::GPUDataBufferCenter::getSSBO found a buffer with a different type");
		}

		return **buffer;
	}

	bool GPUDataBufferCenter::contains(std::string_view p_name)
	{
		return _buffers.contains(std::string(p_name));
	}

	void GPUDataBufferCenter::remove(std::string_view p_name)
	{
		_buffers.erase(std::string(p_name));
	}
}

#endif
