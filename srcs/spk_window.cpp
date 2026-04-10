#include "spk_window.hpp"

#include <utility>

namespace spk
{
	void Window::_enqueueFrameEvent(const spk::Event& p_event)
	{
		std::scoped_lock lock(_pendingEventsMutex);
		_pendingFrameEvents.push_back(p_event);
	}

	void Window::_enqueueMouseEvent(const spk::Event& p_event)
	{
		std::scoped_lock lock(_pendingEventsMutex);
		_pendingMouseEvents.push_back(p_event);
	}

	void Window::_enqueueKeyboardEvent(const spk::Event& p_event)
	{
		std::scoped_lock lock(_pendingEventsMutex);
		_pendingKeyboardEvents.push_back(p_event);
	}

	std::vector<spk::Event> Window::_drainPendingFrameEvents()
	{
		std::vector<spk::Event> result;

		{
			std::scoped_lock lock(_pendingEventsMutex);
			result.swap(_pendingFrameEvents);
		}

		return result;
	}

	std::vector<spk::Event> Window::_drainPendingMouseEvents()
	{
		std::vector<spk::Event> result;

		{
			std::scoped_lock lock(_pendingEventsMutex);
			result.swap(_pendingMouseEvents);
		}

		return result;
	}

	std::vector<spk::Event> Window::_drainPendingKeyboardEvents()
	{
		std::vector<spk::Event> result;

		{
			std::scoped_lock lock(_pendingEventsMutex);
			result.swap(_pendingKeyboardEvents);
		}

		return result;
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

	void Window::_rebuildRenderSnapshot()
	{
		std::shared_ptr<spk::RenderCommandBuilder> newSnapshot = std::make_shared<spk::RenderCommandBuilder>();
		_rootWidget.appendRenderCommands(*newSnapshot);

		std::scoped_lock lock(_renderSnapshotMutex);
		_renderSnapshot = std::move(newSnapshot);
	}

	std::shared_ptr<spk::RenderCommandBuilder> Window::_currentRenderSnapshot() const
	{
		std::scoped_lock lock(_renderSnapshotMutex);
		return _renderSnapshot;
	}

	Window::Window(spk::WindowHost::Configuration p_configuration) :
		_rootWidget(":/" + p_configuration.title + "/RootWidget", nullptr),
		_host(std::move(p_configuration)),
		_renderSnapshot(std::make_shared<spk::RenderCommandBuilder>())
	{
		_rootWidget.activate();

		_frameModule.bind(&_rootWidget);
		_mouseModule.bind(&_rootWidget);
		_keyboardModule.bind(&_rootWidget);
		_updateModule.bind(&_rootWidget);
		_updateModule.bindInputs(&_mouseModule.mouse(), &_keyboardModule.keyboard());

		_frameEventQueueContract = _host.subscribeToFrameEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueFrameEvent(p_event);
			});

		_mouseEventQueueContract = _host.subscribeToMouseEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueMouseEvent(p_event);
			});

		_keyboardEventQueueContract = _host.subscribeToKeyboardEvents(
			[this](const spk::Event& p_event)
			{
				_enqueueKeyboardEvent(p_event);
			});

		_rebuildRenderSnapshot();
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
		std::vector<spk::Event> frameEvents = _drainPendingFrameEvents();
		std::vector<spk::Event> mouseEvents = _drainPendingMouseEvents();
		std::vector<spk::Event> keyboardEvents = _drainPendingKeyboardEvents();

		for (const spk::Event& event : frameEvents)
		{
			_treatFrameEvent(event);
		}

		for (const spk::Event& event : mouseEvents)
		{
			_treatMouseEvent(event);
		}

		for (const spk::Event& event : keyboardEvents)
		{
			_treatKeyboardEvent(event);
		}

		_updateModule.update();
		_rebuildRenderSnapshot();
	}

	void Window::render()
	{
		std::shared_ptr<spk::RenderCommandBuilder> snapshot = _currentRenderSnapshot();

		_host.makeCurrent();

		if (snapshot != nullptr)
		{
			_renderModule.render(*snapshot);
		}

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