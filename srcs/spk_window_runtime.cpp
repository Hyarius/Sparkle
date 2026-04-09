#include "spk_window_runtime.hpp"

namespace spk
{
	void WindowRuntime::_treatEvent(const spk::Event& p_event)
	{
		if (std::holds_alternative<spk::WindowCloseValidatedPayload>(p_event.payload))
		{
			_closureEventProvider.trigger(this);
		}
	}

	WindowRuntime::WindowRuntime(spk::Window::Configuration p_configuration) :
		_rootWidget(":/" + p_configuration.title + "/RootWidget", nullptr),
		_window(std::move(p_configuration))
	{
		_rootWidget.activate();

		_frameModule.bind(&_rootWidget);
		_mouseModule.bind(&_rootWidget);
		_keyboardModule.bind(&_rootWidget);
		_updateModule.bind(&_rootWidget);
		_renderModule.bind(&_rootWidget);

		_frameModule.bindWindow(&_window);
		_mouseModule.bindWindow(&_window);
		_keyboardModule.bindWindow(&_window);

		_onClosureRequestContract = window().subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}

	spk::Window& WindowRuntime::window()
	{
		return _window;
	}

	const spk::Window& WindowRuntime::window() const
	{
		return _window;
	}

	spk::Mouse& WindowRuntime::mouse()
	{
		return _mouseModule.mouse();
	}

	const spk::Mouse& WindowRuntime::mouse() const
	{
		return _mouseModule.mouse();
	}

	spk::Keyboard& WindowRuntime::keyboard()
	{
		return _keyboardModule.keyboard();
	}

	const spk::Keyboard& WindowRuntime::keyboard() const
	{
		return _keyboardModule.keyboard();
	}

	spk::Widget& WindowRuntime::rootWidget()
	{
		return _rootWidget;
	}

	const spk::Widget& WindowRuntime::rootWidget() const
	{
		return _rootWidget;
	}

	void WindowRuntime::pollEvents()
	{
		_window.pumpEvents();
	}

	void WindowRuntime::update(const spk::UpdateTick& p_tick)
	{
		_updateModule.update(p_tick);
	}

	void WindowRuntime::render()
	{
		_window.makeCurrent();
		_renderModule.render();
		_window.present();
	}

	void WindowRuntime::requestClosure()
	{
		_window.requestClosure();
	}

	WindowRuntime::ClosureContract WindowRuntime::subscribeToClosure(ClosureCallback p_callback)
	{
		return _closureEventProvider.subscribe(std::move(p_callback));
	}
}
