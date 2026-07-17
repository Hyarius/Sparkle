#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include "structures/graphics/rendering/context/spk_render_context.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/system/device/window/spk_frame.hpp"
#include "structures/system/win32/spk_winapi_class.hpp"

namespace spk
{
	class PlatformRuntime
	{
	private:
		std::shared_ptr<WindowClass> _windowClass;

	protected:
		template <typename TExpected>
		TExpected &_requireFrame(IFrame &p_frame)
		{
			auto *ptr = dynamic_cast<TExpected *>(&p_frame);
			if (ptr == nullptr)
			{
				throw std::runtime_error(
					std::string("PlatformRuntime: expected ") + typeid(TExpected).name() +
					", got " + typeid(p_frame).name());
			}
			return *ptr;
		}

	public:
		PlatformRuntime();
		virtual ~PlatformRuntime();

		virtual std::unique_ptr<IFrame> createFrame(const spk::Rect2D &p_rect, const std::string &p_title);
		virtual void pollEvents();
		virtual std::unique_ptr<spk::RenderContext> createRenderContext(spk::IFrame &p_frame);
		virtual void waitUntilWorkDone();

		void saveScreenshot(const std::filesystem::path &p_outputPath, const spk::Rect2D &p_rect) const;
		void saveScreenshot(const std::filesystem::path &p_outputPath) const;
	};
}
