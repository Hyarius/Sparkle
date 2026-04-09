#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "spk_contract_provider.hpp"
#include "spk_window_modules.hpp"

namespace spk
{
	class WindowRuntime
	{
	public:
		using ClosureEventProvider = spk::ContractProvider<WindowRuntime*>;
		using ClosureContract = ClosureEventProvider::Contract;
		using ClosureCallback = ClosureEventProvider::Callback;

	private:
		spk::Widget _rootWidget;
		spk::Window _window;

		FrameModule _frameModule;
		MouseModule _mouseModule;
		KeyboardModule _keyboardModule;
		UpdateModule _updateModule;
		RenderModule _renderModule;

		spk::IFrame::Backend::EventContract _onClosureRequestContract;
		ClosureEventProvider _closureEventProvider;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		explicit WindowRuntime(spk::Window::Configuration p_configuration);

		[[nodiscard]] spk::Window& window();
		[[nodiscard]] const spk::Window& window() const;

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

	class WindowRuntimeRegistry
	{
	public:
		using WindowID = std::string;

	private:
		struct Entry
		{
			std::shared_ptr<spk::WindowRuntime> runtime;
			spk::WindowRuntime::ClosureContract closureContract;
		};

		std::unordered_map<WindowID, Entry> _windowRuntimes;

		void _removeWindowRuntime(spk::WindowRuntime* p_windowRuntime);

	public:
		std::shared_ptr<spk::WindowRuntime> createWindowRuntime(const WindowID& p_id, spk::Window::Configuration p_configuration);
		void requestWindowClosing(const WindowID& p_id);
	};
}
