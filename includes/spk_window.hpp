#pragma once

#include "spk_contract_provider.hpp"
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

		spk::IFrame::EventContract _onClosureRequestContract;
		ClosureEventProvider _closureEventProvider;

	private:
		void _treatEvent(const spk::Event& p_event);

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
		void update(const spk::UpdateTick& p_tick);
		void render();
		void requestClosure();

		ClosureContract subscribeToClosure(ClosureCallback p_callback);
	};
}
