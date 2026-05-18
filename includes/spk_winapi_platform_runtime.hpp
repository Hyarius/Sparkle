#pragma once

#ifdef _WIN32

#include <memory>

#include "spk_platform_runtime.hpp"
#include "spk_winapi_class.hpp"

namespace spk::WinAPI
{
	class PlatformRuntime : public spk::IPlatformRuntime
	{
	private:
		std::shared_ptr<Class> _windowClass;

	public:
		PlatformRuntime();
		~PlatformRuntime() override;

		std::unique_ptr<spk::IFrame> createFrame(const spk::Rect2D& p_rect, const std::string& p_title) override;
		void pollEvents() override;
	};
}

#endif
