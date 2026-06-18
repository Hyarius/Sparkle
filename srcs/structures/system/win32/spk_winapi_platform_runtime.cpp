#include "structures/system/win32/spk_winapi_platform_runtime.hpp"

#include "structures/system/win32/spk_winapi_frame.hpp"
#include "structures/system/win32/spk_winapi_window.hpp"

namespace spk
{
	PlatformRuntime::PlatformRuntime() :
		_windowClass(std::make_shared<WindowClass>("Sparkle.WinAPI.Frame", &WindowRuntime::_staticWindowProcedure))
	{
	}

	std::unique_ptr<spk::IFrame> PlatformRuntime::createFrame(const spk::Rect2D& p_rect, const std::string& p_title)
	{
		return std::make_unique<Frame>(_windowClass, p_rect, p_title);
	}

	void PlatformRuntime::pollEvents()
	{
		MSG message{};
		// Win32 window messages are delivered when this thread calls DispatchMessage.
		// That means WndProc runs on this same thread; there is no separate OS-created
		// platform thread. The platform thread is the thread running this pump.
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) == TRUE)
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}
}
