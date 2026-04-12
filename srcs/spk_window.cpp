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
				spk::WindowDestroyedPayload,
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

	void Window::_enqueuePlatformAction(PlatformAction p_action)
	{
		std::scoped_lock lock(_pendingPlatformActionsMutex);
		_pendingPlatformActions.push_back(std::move(p_action));
	}

	void Window::_enqueueRenderAction(RenderAction p_action)
	{
		std::scoped_lock lock(_pendingRenderActionsMutex);

		for (RenderAction& pendingAction : _pendingRenderActions)
		{
			if (pendingAction.type != p_action.type)
			{
				continue;
			}

			pendingAction = std::move(p_action);
			return;
		}

		_pendingRenderActions.push_back(std::move(p_action));
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

	std::vector<Window::PlatformAction> Window::_drainPendingPlatformActions()
	{
		std::vector<PlatformAction> result;

		{
			std::scoped_lock lock(_pendingPlatformActionsMutex);
			result.swap(_pendingPlatformActions);
		}

		return result;
	}

	std::vector<Window::RenderAction> Window::_drainPendingRenderActions()
	{
		std::vector<RenderAction> result;

		{
			std::scoped_lock lock(_pendingRenderActionsMutex);
			result.swap(_pendingRenderActions);
		}

		std::optional<RenderAction> latestResizeAction;
		std::optional<RenderAction> latestVSyncAction;

		for (RenderAction& action : result)
		{
			switch (action.type)
			{
			case RenderActionType::NotifyResize:
				latestResizeAction = std::move(action);
				break;
			case RenderActionType::SetVSync:
				latestVSyncAction = std::move(action);
				break;
			}
		}

		std::vector<RenderAction> coalescedActions;
		if (latestResizeAction.has_value() == true)
		{
			coalescedActions.push_back(std::move(latestResizeAction.value()));
		}
		if (latestVSyncAction.has_value() == true)
		{
			coalescedActions.push_back(std::move(latestVSyncAction.value()));
		}

		return coalescedActions;
	}

	void Window::_treatEvent(const spk::Event& p_event)
	{
		if (_isFrameEvent(p_event) == true)
		{
			_frameModule.pushEvent(p_event);

			if (p_event.holds<spk::WindowCloseRequestedPayload>())
			{
				if (p_event.metadata.isConsumed == false)
				{
					_enqueuePlatformAction(PlatformAction{
						.type = PlatformActionType::ValidateClosure
					});
				}
				else
				{
					_closureRequested.store(false);
				}
				return;
			}

			if (const auto* payload = p_event.getIf<spk::WindowResizedPayload>(); payload != nullptr)
			{
				_enqueueRenderAction(RenderAction{
					.type = RenderActionType::NotifyResize,
					.rect = payload->rect
				});
			}

			if (p_event.holds<spk::WindowDestroyedPayload>())
			{
				_isClosed.store(true);
				_closureNotificationPending.store(true);
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
		_rootWidget.setGeometry(p_configuration.rect.atOrigin());
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

	void Window::executePendingPlatformActions()
	{
		std::vector<PlatformAction> actions = _drainPendingPlatformActions();

		for (const PlatformAction& action : actions)
		{
			switch (action.type)
			{
			case PlatformActionType::RequestClosure:
				_host.requestClosure();
				break;
			case PlatformActionType::ValidateClosure:
				_host.validateClosure();
				break;
			case PlatformActionType::SetTitle:
				if (action.title.has_value() == true)
				{
					_host.setTitle(action.title.value());
				}
				break;
			case PlatformActionType::ResizeFrame:
				if (action.rect.has_value() == true)
				{
					_host.resize(action.rect.value());
				}
				break;
			}
		}

		if (_closureNotificationPending.exchange(false) == true)
		{
			_closureEventProvider.trigger(this);
		}

		if (_isClosed.load() == true &&
			_renderResourcesReleased.load() == true &&
			_platformResourcesReleased.exchange(true) == false)
		{
			_host.releaseFrame();
		}
	}

	void Window::update()
	{
		if (_isClosed.load() == true)
		{
			return;
		}

		std::vector<spk::Event> events = _drainPendingEvents();

		for (const spk::Event& event : events)
		{
			_treatEvent(event);

			if (_isClosed.load() == true)
			{
				return;
			}
		}

		_updateModule.update();
		_rebuildRenderSnapshot();
	}

	void Window::render()
	{
		if (_isClosed.load() == true)
		{
			if (_renderResourcesReleased.exchange(true) == false)
			{
				_host.releaseRenderContext();
			}
			return;
		}

		std::shared_ptr<spk::RenderCommandBuilder> snapshot = _currentRenderSnapshot();
		std::vector<RenderAction> renderActions = _drainPendingRenderActions();

		if (_host.makeCurrent() == false)
		{
			return;
		}

		for (const RenderAction& action : renderActions)
		{
			switch (action.type)
			{
			case RenderActionType::NotifyResize:
				if (action.rect.has_value() == true)
				{
					_host.notifyFrameResized(action.rect.value());
				}
				break;
			case RenderActionType::SetVSync:
				if (action.vSyncEnabled.has_value() == true)
				{
					_host.setVSync(action.vSyncEnabled.value());
				}
				break;
			}
		}

		if (snapshot != nullptr)
		{
			_renderModule.render(*snapshot);
		}

		_host.present();
	}

	void Window::requestClosure()
	{
		if (_isClosed.load() == true)
		{
			return;
		}

		if (_closureRequested.exchange(true) == true)
		{
			return;
		}

		_enqueuePlatformAction(PlatformAction{
			.type = PlatformActionType::RequestClosure
		});

		if (_host.isPlatformThread() == true)
		{
			executePendingPlatformActions();
		}
	}

	bool Window::isClosed() const
	{
		return _isClosed.load();
	}

	bool Window::isReadyForDisposal() const
	{
		return (
			_isClosed.load() == true &&
			_renderResourcesReleased.load() == true &&
			_platformResourcesReleased.load() == true);
	}

	Window::ClosureContract Window::subscribeToClosure(ClosureCallback p_callback)
	{
		return _closureEventProvider.subscribe(std::move(p_callback));
	}
}
