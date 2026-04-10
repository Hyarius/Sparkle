#pragma once

#include <memory>
#include <string>

#include "spk_contract_provider.hpp"
#include "spk_events.hpp"
#include "spk_rect_2d.hpp"
#include "spk_render_context.hpp"

namespace spk
{
	class IFrame
	{
	public:
		using EventContractProvider = spk::ContractProvider<const spk::Event&>;
		using EventContract = EventContractProvider::Contract;
		using EventCallback = EventContractProvider::Callback;

	private:
		EventContractProvider _mouseEventContractProvider;
		EventContractProvider _keyboardEventContractProvider;
		EventContractProvider _frameEventContractProvider;

	protected:
		void _emitMouseEvent(const spk::Event& p_event);
		void _emitKeyboardEvent(const spk::Event& p_event);
		void _emitFrameEvent(const spk::Event& p_event);

	public:
		virtual ~IFrame();

		virtual void resize(const spk::Rect2D& p_rect) = 0;
		virtual void setTitle(const std::string& p_title) = 0;
		virtual void requestClosure() = 0;
		virtual std::unique_ptr<IRenderContext> createRenderContext(IRenderContext::Backend& p_backend) = 0;

		[[nodiscard]] virtual spk::Rect2D rect() const = 0;
		[[nodiscard]] virtual std::string title() const = 0;

		EventContract subscribeToMouseEvents(EventCallback p_callback);
		EventContract subscribeToKeyboardEvents(EventCallback p_callback);
		EventContract subscribeToFrameEvents(EventCallback p_callback);
	};
}
