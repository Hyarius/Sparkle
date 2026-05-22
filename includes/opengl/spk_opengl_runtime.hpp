#pragma once

#ifdef _WIN32

#include <filesystem>

#include "opengl/spk_gpu_platform_runtime.hpp"

namespace spk::OpenGL
{
	class Runtime : public spk::IGPUPlatformRuntime
	{
	public:
		~Runtime() override;

		std::unique_ptr<spk::IRenderContext> createRenderContext(spk::IFrame& p_frame) override;
		void waitUntilWorkDone() override;

		void saveScreenshot(const std::filesystem::path& p_outputPath, const spk::Rect2D& p_rect) const;
		void saveScreenshot(const std::filesystem::path& p_outputPath) const;
	};
}

#endif
