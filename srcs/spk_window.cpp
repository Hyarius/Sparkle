#include "spk_window.hpp"

#include <utility>

namespace spk
{
	void Window::_enqueueEvent(const spk::Event& p_event)
	{
		std::scoped_lock lock(_pendingEventsMutex);
		_pendingEvents.push_back(p_event);
	}

	std::vector<spk::Event> Window::_drainPendingEvents()
	{
		std::vector<spk::Event> result;

		{
			std::scoped_lock lock(_pendingEventsMutex);
			result.swap(_pendingEvents);
		}

		return result;
	}

	bool Window::_isFrameEvent(const spk::Event& p_event)
	{
		return (
			p_event.holds<spk::WindowCloseRequestedPayload>() ||
			p_event.holds<spk::WindowCloseValidatedPayload>() ||
			p_event.holds<spk::WindowMovedPayload>() ||
			p_event.holds<spk::WindowResizedPayload>() ||
			p_event.holds<spk::WindowFocusGainedPayload>() ||
			p_event.holds<spk::WindowFocusLostPayload>() ||
			p_event.holds<spk::WindowShownPayload>() ||
			p_event.holds<spk::WindowHiddenPayload>());
	}

	bool Window::_isMouseEvent(const spk::Event& p_event)
	{
		return (
			p_event.holds<spk::MouseEnteredPayload>() ||
			p_event.holds<spk::MouseLeftPayload>() ||
			p_event.holds<spk::MouseMovedPayload>() ||
			p_event.holds<spk::MouseWheelScrolledPayload>() ||
			p_event.holds<spk::MouseButtonPressedPayload>() ||
			p_event.holds<spk::MouseButtonReleasedPayload>() ||
			p_event.holds<spk::MouseButtonDoubleClickedPayload>());
	}

	bool Window::_isKeyboardEvent(const spk::Event& p_event)
	{
		return (
			p_event.holds<spk::KeyPressedPayload>() ||
			p_event.holds<spk::KeyReleasedPayload>() ||
			p_event.holds<spk::TextInputPayload>());
	}

	void Window::_treatFrameEvent(const spk::Event& p_event)
	{
		_frameModule.pushEvent(p_event);

		if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
		{
			_host.notifyFrameResized(payload->rect);
		}

		if (p_event.holds<spk::WindowCloseValidatedPayload>())
		{
			_closureEventProvider.trigger(this);
		}
	}

	void Window::_treatMouseEvent(const spk::Event& p_event)
	{
		_mouseModule.pushEvent(p_event);
	}

	void Window::_treatKeyboardEvent(const spk::Event& p_event)
	{
		_keyboardModule.pushEvent(p_event);
	}

	void Window::_dispatchEvent(const spk::Event& p_event)
	{
		if (_isFrameEvent(p_event))
		{
			_treatFrameEvent(p_event);
			return;
		}

		if (_isMouseEvent(p_event))
		{
			_treatMouseEvent(p_event);
			return;
		}

		if (_isKeyboardEvent(p_event))
		{
			_treatKeyboardEvent(p_event);
			return;
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
		_updateModule.bindInputs(&_mouseModule.mouse(), &_keyboardModule.keyboard());
		_renderModule.bind(&_rootWidget);

		_frameEventQueueContract = _host.subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueEvent(p_event);
			});

		_mouseEventQueueContract = _host.subscribeToMouseEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueEvent(p_event);
			});

		_keyboardEventQueueContract = _host.subscribeToKeyboardEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueEvent(p_event);
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

	void Window::update()
	{
		std::vector<spk::Event> pendingEvents = _drainPendingEvents();

		for (const spk::Event& event : pendingEvents)
		{
			_dispatchEvent(event);
		}

		_updateModule.update();
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