#pragma once

#include <memory>
#include <string>

#include "spk_platform_runtime.hpp"
#include "spk_rect_2d.hpp"
#include "spk_render_context.hpp"

namespace spk
{
	class WindowHost
	{
	public:
		struct Configuration
		{
			spk::Rect2D rect;
			std::string title;
			std::shared_ptr<IPlatformRuntime> platformRuntime = nullptr;
			std::unique_ptr<IRenderContext::Backend> renderBackend = nullptr;
		};

	private:
		std::shared_ptr<IPlatformRuntime> _platformRuntime;
		std::unique_ptr<IRenderContext::Backend> _renderBackend;
		std::unique_ptr<IFrame> _frame;
		std::unique_ptr<IRenderContext> _renderContext;

	private:
		static std::shared_ptr<IPlatformRuntime> _createDefaultPlatformRuntime();
		static std::unique_ptr<IRenderContext::Backend> _createDefaultRenderBackend();

	public:
		explicit WindowHost(Configuration p_configuration);
		~WindowHost();

		void resize(const spk::Rect2D& p_rect);
		void notifyFrameResized(const spk::Rect2D& p_rect);
		void setTitle(const std::string& p_title);
		void requestClosure();

		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] std::string title() const;

		void makeCurrent();
		void present();
		void setVSync(bool p_enabled);
		void pollEvents();

		IFrame::EventContract subscribeToMouseEvents(IFrame::EventCallback p_callback);
		IFrame::EventContract subscribeToKeyboardEvents(IFrame::EventCallback p_callback);
		IFrame::EventContract subscribeToFrameEvents(IFrame::EventCallback p_callback);
	};
}
