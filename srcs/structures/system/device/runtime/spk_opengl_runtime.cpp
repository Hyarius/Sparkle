#include "structures/system/device/runtime/spk_opengl_runtime.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <Windows.h>
#include <GL/glew.h>

#include <stb_image_write.h>

#include "structures/graphics/opengl/spk_opengl_render_context.hpp"
#include "structures/system/win32/spk_winapi_frame.hpp"

namespace spk
{
	std::unique_ptr<spk::RenderContext> GPUPlatformRuntime::createRenderContext(spk::IFrame& p_frame)
	{
		spk::Frame& frame = requireFrame<spk::Frame>(p_frame);
		return std::make_unique<RenderContext>(frame);
	}

	void GPUPlatformRuntime::waitUntilWorkDone()
	{
		glFinish();
	}

	void GPUPlatformRuntime::saveScreenshot(const std::filesystem::path& p_outputPath, const spk::Rect2D& p_rect) const
	{
		const int width = static_cast<int>(p_rect.width());
		const int height = static_cast<int>(p_rect.height());
		if (width <= 0 || height <= 0)
		{
			throw std::runtime_error("spk::GPUPlatformRuntime::saveScreenshot requires a non-empty capture rect");
		}

		std::filesystem::create_directories(p_outputPath.parent_path());

		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
		std::vector<std::uint8_t> flipped(pixels.size());

		GLint readFramebuffer = 0;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
		glReadBuffer(readFramebuffer == 0 ? GL_BACK : GL_COLOR_ATTACHMENT0);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(p_rect.x(), p_rect.y(), width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		const std::size_t stride = static_cast<std::size_t>(width) * 4;
		for (int y = 0; y < height; ++y)
		{
			const std::size_t sourceOffset = static_cast<std::size_t>(height - 1 - y) * stride;
			const std::size_t destinationOffset = static_cast<std::size_t>(y) * stride;
			std::copy(pixels.begin() + sourceOffset, pixels.begin() + sourceOffset + stride, flipped.begin() + destinationOffset);
		}

		if (stbi_write_png(p_outputPath.string().c_str(), width, height, 4, flipped.data(), width * 4) == 0)
		{
			throw std::runtime_error("spk::GPUPlatformRuntime::saveScreenshot failed to write [" + p_outputPath.string() + "]");
		}
	}

	void GPUPlatformRuntime::saveScreenshot(const std::filesystem::path& p_outputPath) const
	{
		GLint viewport[4] = {0, 0, 0, 0};
		glGetIntegerv(GL_VIEWPORT, viewport);

		saveScreenshot(
			p_outputPath,
			spk::Rect2D(
				static_cast<int>(viewport[0]),
				static_cast<int>(viewport[1]),
				static_cast<std::size_t>(viewport[2]),
				static_cast<std::size_t>(viewport[3])));
	}
}
