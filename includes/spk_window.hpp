#pragma once

#include <memory>
#include <string>

#include "spk_frame.hpp"
#include "spk_rect_2d.hpp"
#include "spk_render_context.hpp"

namespace spk
{
	class Window
	{
	public:
		struct Configuration
		{
			spk::Rect2D rect;
			std::string title;
			std::unique_ptr<IFrame::Backend> frameBackend = nullptr;
			std::unique_ptr<IRenderContext::Backend> renderBackend = nullptr;
		};

	private:
		std::unique_ptr<IFrame::Backend> _frameBackend;
		std::unique_ptr<IRenderContext::Backend> _renderBackend;
		std::unique_ptr<IFrame> _frame;
		std::unique_ptr<IRenderContext> _renderContext;

	private:
		static std::unique_ptr<IFrame::Backend> _createDefaultFrameBackend();
		static std::unique_ptr<IRenderContext::Backend> _createDefaultRenderBackend();

	public:
		explicit Window(Configuration p_configuration);
		~Window();

		void resize(const spk::Rect2D& p_rect);
		void notifyFrameResized(const spk::Rect2D& p_rect);
		void setTitle(const std::string& p_title);
		void requestClosure();

		[[nodiscard]] spk::Rect2D rect() const;
		[[nodiscard]] std::string title() const;

		void makeCurrent();
		void present();
		void setVSync(bool p_enabled);
		void pumpEvents();

		IFrame::Backend::EventContract subscribeToMouseEvents(IFrame::Backend::EventCallback p_callback);
		IFrame::Backend::EventContract subscribeToKeyboardEvents(IFrame::Backend::EventCallback p_callback);
		IFrame::Backend::EventContract subscribeToFrameEvents(IFrame::Backend::EventCallback p_callback);
	};
}
