#include "spk_window.hpp"

namespace spk
{
	void Window::_treatEvent(const spk::Event& p_event)
	{
		if (std::holds_alternative<spk::WindowCloseValidatedPayload>(p_event.payload))
		{
			_closureEventProvider.trigger(this);
		}
	}

	Window::Window(spk::WindowHost::Configuration p_configuration) :
		_rootWidget(":/" + p_configuration.title + "/RootWidget", nullptr),
		_host(std::move(p_configuration))
	{
		_rootWidget.activate();

		_frameModule.bind(&_rootWidget);
		_mouseModule.bind(&_rootWidget);
		_keyboardModule.bind(&_rootWidget);
		_updateModule.bind(&_rootWidget);
		_renderModule.bind(&_rootWidget);

		_frameModule.bindWindowHost(&_host);
		_mouseModule.bindWindowHost(&_host);
		_keyboardModule.bindWindowHost(&_host);

		_onClosureRequestContract = host().subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_treatEvent(p_event);
			});
	}

	spk::WindowHost& Window::host()
	{
		return _host;
	}

	const spk::WindowHost& Window::host() const
	{
		return _host;
	}

	spk::Mouse& Window::mouse()
	{
		return _mouseModule.mouse();
	}

	const spk::Mouse& Window::mouse() const
	{
		return _mouseModule.mouse();
	}

	spk::Keyboard& Window::keyboard()
	{
		return _keyboardModule.keyboard();
	}

	const spk::Keyboard& Window::keyboard() const
	{
		return _keyboardModule.keyboard();
	}

	spk::Widget& Window::rootWidget()
	{
		return _rootWidget;
	}

	const spk::Widget& Window::rootWidget() const
	{
		return _rootWidget;
	}

	void Window::pollEvents()
	{
		_host.pollEvents();
	}

	void Window::update(const spk::UpdateTick& p_tick)
	{
		_updateModule.update(p_tick);
	}

	void Window::render()
	{
		_host.makeCurrent();
		_renderModule.render();
		_host.present();
	}

	void Window::requestClosure()
	{
		_host.requestClosure();
	}

	Window::ClosureContract Window::subscribeToClosure(ClosureCallback p_callback)
	{
		return _closureEventProvider.subscribe(std::move(p_callback));
	}
}
