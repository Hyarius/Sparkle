#pragma once

#include <memory>
#include <string>

#include "spk_frame.hpp"
#include "spk_gpu_platform_runtime.hpp"
#include "spk_rect_2d.hpp"
#include "spk_render_context.hpp"

namespace spk
{
	class WindowHost
	{
	private:
		std::unique_ptr<IFrame> _frame;
		std::unique_ptr<IRenderContext> _renderContext;

	public:
		WindowHost(std::unique_ptr<IFrame> p_frame, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime);
		~WindowHost();

		void resize(const spk::Rect2D& p_rect);
		void notifyFrameResized(const spk::Rect2D& p_rect);
		void setTitle(const std::string& p_title);
		void requestClosure();
		void validateClosure();

		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] std::string title() const;

		void makeCurrent();
		void present();
		void setVSync(bool p_enabled);

		IFrame::EventContract subscribeToMouseEvents(IFrame::EventCallback p_callback);
		IFrame::EventContract subscribeToKeyboardEvents(IFrame::EventCallback p_callback);
		IFrame::EventContract subscribeToFrameEvents(IFrame::EventCallback p_callback);
	};
}
