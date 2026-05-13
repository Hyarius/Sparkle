#pragma once

#include <memory>
#include <string>

#include "spk_contract_provider.hpp"
#include "spk_events.hpp"
#include "spk_rect_2d.hpp"
#include "spk_surface_state.hpp"

namespace spk
{
	class IFrame
	{
	public:
		using MouseEventContractProvider = spk::ContractProvider<const spk::MouseEventRecord&>;
		using MouseEventContract = MouseEventContractProvider::Contract;
		using MouseEventCallback = MouseEventContractProvider::Callback;

		using KeyboardEventContractProvider = spk::ContractProvider<const spk::KeyboardEventRecord&>;
		using KeyboardEventContract = KeyboardEventContractProvider::Contract;
		using KeyboardEventCallback = KeyboardEventContractProvider::Callback;

		using FrameEventContractProvider = spk::ContractProvider<const spk::FrameEventRecord&>;
		using FrameEventContract = FrameEventContractProvider::Contract;
		using FrameEventCallback = FrameEventContractProvider::Callback;

	private:
		MouseEventContractProvider _mouseEventContractProvider;
		KeyboardEventContractProvider _keyboardEventContractProvider;
		FrameEventContractProvider _frameEventContractProvider;
		std::shared_ptr<ISurfaceState> _surfaceState;

	protected:
		explicit IFrame(std::shared_ptr<ISurfaceState> p_surfaceState);

		void _emitMouseEvent(const spk::MouseEventRecord& p_event);
		void _emitKeyboardEvent(const spk::KeyboardEventRecord& p_event);
		void _emitFrameEvent(const spk::FrameEventRecord& p_event);
		void _invalidateSurfaceState();

	public:
		virtual ~IFrame();

		virtual void resize(const spk::Rect2D& p_rect) = 0;
		virtual void setTitle(const std::string& p_title) = 0;
		virtual void requestClosure() = 0;
		virtual void validateClosure() = 0;

		[[nodiscard]] virtual spk::Rect2D rect() const = 0;
		[[nodiscard]] virtual std::string title() const = 0;
		[[nodiscard]] std::shared_ptr<ISurfaceState> surfaceState() const;

		MouseEventContract subscribeToMouseEvents(MouseEventCallback p_callback);
		KeyboardEventContract subscribeToKeyboardEvents(KeyboardEventCallback p_callback);
		FrameEventContract subscribeToFrameEvents(FrameEventCallback p_callback);
	};
}
