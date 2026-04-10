#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "spk_contract_provider.hpp"
#include "spk_render_command_builder.hpp"
#include "spk_window_host.hpp"
#include "spk_window_modules.hpp"

namespace spk
{
	class Window
	{
	public:
		using ClosureEventProvider = spk::ContractProvider<Window*>;
		using ClosureContract = ClosureEventProvider::Contract;
		using ClosureCallback = ClosureEventProvider::Callback;

	private:
		spk::Widget _rootWidget;
		spk::WindowHost _host;

		FrameModule _frameModule;
		MouseModule _mouseModule;
		KeyboardModule _keyboardModule;
		UpdateModule _updateModule;
		RenderModule _renderModule;

		mutable std::mutex _pendingEventsMutex;
		std::vector<spk::Event> _pendingFrameEvents;
		std::vector<spk::Event> _pendingMouseEvents;
		std::vector<spk::Event> _pendingKeyboardEvents;

		mutable std::mutex _renderSnapshotMutex;
		std::shared_ptr<spk::RenderCommandBuilder> _renderSnapshot;

		spk::IFrame::EventContract _frameEventQueueContract;
		spk::IFrame::EventContract _mouseEventQueueContract;
		spk::IFrame::EventContract _keyboardEventQueueContract;

		ClosureEventProvider _closureEventProvider;

	private:
		void _enqueueFrameEvent(const spk::Event& p_event);
		void _enqueueMouseEvent(const spk::Event& p_event);
		void _enqueueKeyboardEvent(const spk::Event& p_event);

		[[nodiscard]] std::vector<spk::Event> _drainPendingFrameEvents();
		[[nodiscard]] std::vector<spk::Event> _drainPendingMouseEvents();
		[[nodiscard]] std::vector<spk::Event> _drainPendingKeyboardEvents();

		void _treatFrameEvent(const spk::Event& p_event);
		void _treatMouseEvent(const spk::Event& p_event);
		void _treatKeyboardEvent(const spk::Event& p_event);

		void _rebuildRenderSnapshot();
		[[nodiscard]] std::shared_ptr<spk::RenderCommandBuilder> _currentRenderSnapshot() const;

	public:
		explicit Window(spk::WindowHost::Configuration p_configuration);

		[[nodiscard]] spk::WindowHost& host();
		[[nodiscard]] const spk::WindowHost& host() const;

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;

		[[nodiscard]] spk::Widget& rootWidget();
		[[nodiscard]] const spk::Widget& rootWidget() const;

		void pollEvents();
		void update();
		void render();
		void requestClosure();

		ClosureContract subscribeToClosure(ClosureCallback p_callback);
	};
}