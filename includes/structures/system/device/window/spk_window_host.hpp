#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "structures/system/device/window/spk_frame.hpp"
#include "structures/system/device/runtime/spk_opengl_runtime.hpp"
#include "structures/math/spk_rect_2d.hpp"
#include "structures/graphics/rendering/context/spk_render_context.hpp"

namespace spk
{
	class WindowHost
	{
	private:
		std::unique_ptr<IFrame> _frame;
		std::shared_ptr<GPUPlatformRuntime> _gpuPlatformRuntime;
		std::unique_ptr<RenderContext> _renderContext;
		std::thread::id _platformThreadID;
		mutable std::mutex _renderThreadMutex;
		std::optional<std::thread::id> _renderThreadID;

	private:
		void _bindOrValidatePlatformThread(const char* p_operation) const;
		void _bindOrValidateRenderThreadLocked(const char* p_operation);
		void _validateRenderThreadLocked(const char* p_operation) const;
		[[nodiscard]] bool _ensureRenderContextLocked();

	public:
		WindowHost(std::unique_ptr<IFrame> p_frame, std::shared_ptr<GPUPlatformRuntime> p_gpuPlatformRuntime);
		~WindowHost();

		[[nodiscard]] bool isPlatformThread() const;
		[[nodiscard]] bool isRenderThread() const;

		void resize(const spk::Rect2D& p_rect);
		void notifyFrameResized(const spk::Rect2D& p_rect);
		void setTitle(const std::string& p_title);
		void setCursor(const std::string& p_name);
		void requestClosure();
		void validateClosure();
		void releaseFrame();
		void releaseRenderContext();

		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] std::string title() const;

		[[nodiscard]] bool makeCurrent();
		[[nodiscard]] RenderContext& renderContext();
		void present();
		void setVSync(bool p_enabled);

		IFrame::MouseEventContract subscribeToMouseEvents(IFrame::MouseEventCallback p_callback);
		IFrame::KeyboardEventContract subscribeToKeyboardEvents(IFrame::KeyboardEventCallback p_callback);
		IFrame::FrameEventContract subscribeToFrameEvents(IFrame::FrameEventCallback p_callback);
	};
}
