#pragma once

#ifdef _WIN32

#include <memory>
#include <string>

#include "math/spk_rect_2d.hpp"
#include "window/spk_frame.hpp"
#include "winapi/spk_winapi_class.hpp"

namespace spk
{
	class PlatformRuntime
	{
	private:
		std::shared_ptr<WindowClass> _windowClass;

	public:
		PlatformRuntime();
		virtual ~PlatformRuntime();

		virtual std::unique_ptr<IFrame> createFrame(const spk::Rect2D& p_rect, const std::string& p_title);
		virtual void pollEvents();
	};
}

#endif
