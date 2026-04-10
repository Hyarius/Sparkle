#include "spk_window.hpp"

#include <stdexcept>
#include <utility>

namespace spk
{
	namespace
	{
		std::unique_ptr<spk::IFrame> _createFrame(const std::shared_ptr<spk::IPlatformRuntime>& p_platformRuntime, const spk::Rect2D& p_rect, const std::string& p_title)
		{
			if (p_platformRuntime == nullptr)
			{
				throw std::runtime_error("spk::Window requires a valid spk::IPlatformRuntime to create its frame");
			}

			std::unique_ptr<spk::IFrame> result = p_platformRuntime->createFrame(p_rect, p_title);

			if (result == nullptr)
			{
				throw std::runtime_error("spk::Window failed to create its frame");
			}

			return result;
		}

		template <typename... TPayloadTypes>
		bool _holdsAny(const spk::Event& p_event)
		{
			return (p_event.holds<TPayloadTypes>() || ...);
		}

		bool _isFrameEvent(const spk::Event& p_event)
		{
			return _holdsAny<
				spk::WindowCloseRequestedPayload,
				spk::WindowCloseValidatedPayload,
				spk::WindowMovedPayload,
				spk::WindowResizedPayload,
				spk::WindowFocusGainedPayload,
				spk::WindowFocusLostPayload,
				spk::WindowShownPayload,
				spk::WindowHiddenPayload>(p_event);
		}

		bool _isMouseEvent(const spk::Event& p_event)
		{
			return _holdsAny<
				spk::MouseEnteredPayload,
				spk::MouseLeftPayload,
				spk::MouseMovedPayload,
				spk::MouseWheelScrolledPayload,
				spk::MouseButtonPressedPayload,
				spk::MouseButtonReleasedPayload,
				spk::MouseButtonDoubleClickedPayload>(p_event);
		}

		bool _isKeyboardEvent(const spk::Event& p_event)
		{
			return _holdsAny<
				spk::KeyPressedPayload,
				spk::KeyReleasedPayload,
				spk::TextInputPayload>(p_event);
		}
	}

	void Window::_enqueueEvent(const spk::Event& p_event)
	{
		std::scoped_lock lock(_pendingEventsMutex);
		_pendingEvents.push_back(p_event);
	}

	void Window::_recordPendingFrameResize(const spk::Rect2D& p_rect)
	{
		std::scoped_lock lock(_pendingFrameResizeMutex);
		_pendingFrameResize = p_rect;
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

	std::optional<spk::Rect2D> Window::_consumePendingFrameResize()
	{
		std::optional<spk::Rect2D> result;

		{
			std::scoped_lock lock(_pendingFrameResizeMutex);
			result = _pendingFrameResize;
			_pendingFrameResize.reset();
		}

		return result;
	}

	void Window::_treatEvent(const spk::Event& p_event)
	{
		if (_isFrameEvent(p_event) == true)
		{
			_frameModule.pushEvent(p_event);

			if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
			{
				_recordPendingFrameResize(payload->rect);
			}

			if (p_event.holds<spk::WindowCloseValidatedPayload>())
			{
				_closureEventProvider.trigger(this);
			}
			return;
		}

		if (_isMouseEvent(p_event) == true)
		{
			_mouseModule.pushEvent(p_event);
			return;
		}

		if (_isKeyboardEvent(p_event) == true)
		{
			_keyboardModule.pushEvent(p_event);
			return;
		}

		throw std::runtime_error("spk::Window encountered an event with an unsupported category");
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

	Window::Window(std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, Configuration p_configuration) :
		_rootWidget(":/" + p_configuration.title + "/RootWidget", nullptr),
		_host(
			_createFrame(p_platformRuntime, p_configuration.rect, p_configuration.title),
			std::move(p_gpuPlatformRuntime)),
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

	void Window::update()
	{
		std::vector<spk::Event> events = _drainPendingEvents();

		for (const spk::Event& event : events)
		{
			_treatEvent(event);
		}

		_updateModule.update();
		_rebuildRenderSnapshot();
	}

	void Window::render()
	{
		std::shared_ptr<spk::RenderCommandBuilder> snapshot = _currentRenderSnapshot();
		std::optional<spk::Rect2D> pendingFrameResize = _consumePendingFrameResize();

		_host.makeCurrent();

		if (pendingFrameResize.has_value() == true)
		{
			_host.notifyFrameResized(pendingFrameResize.value());
		}

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
