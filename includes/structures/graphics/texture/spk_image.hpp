#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "structures/graphics/spk_texture.hpp"

namespace spk
{
	class Image : public spk::Texture
	{
	public:
		Image();
		explicit Image(const std::filesystem::path &p_path);

		void loadFromFile(const std::filesystem::path &p_path);
		void loadFromData(const std::vector<uint8_t> &p_data);

	private:
		static Format _determineFormat(int p_channels);
	};
}
