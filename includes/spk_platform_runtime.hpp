#pragma once

#include <memory>
#include <string>

#include "spk_frame.hpp"
#include "spk_rect_2d.hpp"

namespace spk
{
	class IPlatformRuntime
	{
	public:
		virtual ~IPlatformRuntime();

		virtual std::unique_ptr<IFrame> createFrame(const spk::Rect2D& p_rect, const std::string& p_title) = 0;
		virtual void pollEvents() = 0;
	};
}
