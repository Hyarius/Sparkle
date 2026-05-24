#pragma once

#include <filesystem>
#include <memory>

#include "rendering/spk_render_context.hpp"
#include "window/spk_frame.hpp"

namespace spk
{
	class GPUPlatformRuntime
	{
	protected:
		template<typename TExpected>
		TExpected& requireFrame(IFrame& p_frame)
		{
			auto* ptr = dynamic_cast<TExpected*>(&p_frame);
			if (ptr == nullptr)
				throw std::runtime_error(
					std::string("GPUPlatformRuntime: expected ") + typeid(TExpected).name() +
					", got " + typeid(p_frame).name());
			return *ptr;
		}

	public:
		virtual ~GPUPlatformRuntime() = default;

		virtual std::unique_ptr<spk::RenderContext> createRenderContext(spk::IFrame& p_frame);
		virtual void waitUntilWorkDone();

#ifdef _WIN32
		void saveScreenshot(const std::filesystem::path& p_outputPath, const spk::Rect2D& p_rect) const;
		void saveScreenshot(const std::filesystem::path& p_outputPath) const;
#endif
	};
}
