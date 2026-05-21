#pragma once

#if defined(SPARKLE_GPU_BACKEND_OPENGL)

#include <filesystem>
#include <vector>
#include <cstdint>

#include "opengl/spk_opengl_texture.hpp"

namespace spk
{
	class Image : public spk::OpenGL::Texture
	{
	public:
		Image();
		explicit Image(const std::filesystem::path& p_path);

		void loadFromFile(const std::filesystem::path& p_path);
		void loadFromData(const std::vector<uint8_t>& p_data);

	private:
		static Format _determineFormat(int p_channels);
	};
}

#endif
