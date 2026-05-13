#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

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
		std::shared_ptr<IGPUPlatformRuntime> _gpuPlatformRuntime;
		std::unique_ptr<IRenderContext> _renderContext;
		std::thread::id _platformThreadID;
		mutable std::mutex _renderThreadMutex;
		std::optional<std::thread::id> _renderThreadID;

	private:
		void _bindOrValidatePlatformThread(const char* p_operation) const;
		void _bindOrValidateRenderThreadLocked(const char* p_operation);
		[[nodiscard]] bool _ensureRenderContextLocked();

	public:
		WindowHost(std::unique_ptr<IFrame> p_frame, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime);
		~WindowHost();

		[[nodiscard]] bool isPlatformThread() const;
		[[nodiscard]] bool isRenderThread() const;

		void resize(const spk::Rect2D& p_rect);
		void notifyFrameResized(const spk::Rect2D& p_rect);
		void setTitle(const std::string& p_title);
		void requestClosure();
		void validateClosure();
		void releaseFrame();
		void releaseRenderContext();

		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] std::string title() const;

		[[nodiscard]] bool makeCurrent();
		void present();
		void setVSync(bool p_enabled);

		IFrame::MouseEventContract subscribeToMouseEvents(IFrame::MouseEventCallback p_callback);
		IFrame::KeyboardEventContract subscribeToKeyboardEvents(IFrame::KeyboardEventCallback p_callback);
		IFrame::FrameEventContract subscribeToFrameEvents(IFrame::FrameEventCallback p_callback);
	};
}
