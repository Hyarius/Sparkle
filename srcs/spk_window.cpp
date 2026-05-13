#include "spk_window.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace spk
{
	namespace
	{
		std::unique_ptr<spk::IFrame> _createFrame(const std::shared_ptr<spk::IPlatformRuntime> &p_platformRuntime, const spk::Rect2D &p_rect, const std::string &p_title)
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

	}

	void Window::_enqueueFrameEvent(spk::FrameEventRecord p_event)
	{
		spk::setEventSequence(p_event, _nextEventSequence.fetch_add(1));

		std::scoped_lock lock(_pendingFrameEventsMutex);
		_pendingFrameEvents.push_back(std::move(p_event));
	}

	void Window::_enqueueMouseEvent(spk::MouseEventRecord p_event)
	{
		spk::setEventSequence(p_event, _nextEventSequence.fetch_add(1));

		std::scoped_lock lock(_pendingMouseEventsMutex);
		_pendingMouseEvents.push_back(std::move(p_event));
	}

	void Window::_enqueueKeyboardEvent(spk::KeyboardEventRecord p_event)
	{
		spk::setEventSequence(p_event, _nextEventSequence.fetch_add(1));

		std::scoped_lock lock(_pendingKeyboardEventsMutex);
		_pendingKeyboardEvents.push_back(std::move(p_event));
	}

	void Window::_enqueuePlatformAction(PlatformAction p_action)
	{
		std::scoped_lock lock(_pendingPlatformActionsMutex);
		_pendingPlatformActions.push_back(std::move(p_action));
	}

	void Window::_enqueueRenderAction(RenderAction p_action)
	{
		std::scoped_lock lock(_pendingRenderActionsMutex);

		for (RenderAction &pendingAction : _pendingRenderActions)
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

	void Window::_drainPendingEvents()
	{
		_eventsToProcess.clear();
		_frameEventsToProcess.clear();
		_mouseEventsToProcess.clear();
		_keyboardEventsToProcess.clear();

		{
			std::scoped_lock lock(_pendingFrameEventsMutex);
			_frameEventsToProcess.swap(_pendingFrameEvents);
		}

		{
			std::scoped_lock lock(_pendingMouseEventsMutex);
			_mouseEventsToProcess.swap(_pendingMouseEvents);
		}

		{
			std::scoped_lock lock(_pendingKeyboardEventsMutex);
			_keyboardEventsToProcess.swap(_pendingKeyboardEvents);
		}

		_eventsToProcess.reserve(_frameEventsToProcess.size() + _mouseEventsToProcess.size() + _keyboardEventsToProcess.size());

		for (size_t i = 0; i < _frameEventsToProcess.size(); ++i)
		{
			_eventsToProcess.push_back(EventReference{
				.family = EventFamily::Frame,
				.index = i,
				.sequence = spk::eventSequence(_frameEventsToProcess[i])
			});
		}

		for (size_t i = 0; i < _mouseEventsToProcess.size(); ++i)
		{
			_eventsToProcess.push_back(EventReference{
				.family = EventFamily::Mouse,
				.index = i,
				.sequence = spk::eventSequence(_mouseEventsToProcess[i])
			});
		}

		for (size_t i = 0; i < _keyboardEventsToProcess.size(); ++i)
		{
			_eventsToProcess.push_back(EventReference{
				.family = EventFamily::Keyboard,
				.index = i,
				.sequence = spk::eventSequence(_keyboardEventsToProcess[i])
			});
		}

		std::sort(
			_eventsToProcess.begin(),
			_eventsToProcess.end(),
			[](const EventReference& p_left, const EventReference& p_right)
			{
				return p_left.sequence < p_right.sequence;
			});
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

		for (RenderAction &action : result)
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

	void Window::_treatFrameEvent(spk::FrameEventRecord &p_event)
	{
		const bool isConsumed = _frameModule.pushEvent(p_event);

		if (const auto* event = spk::getIf<spk::WindowCloseRequestedRecord>(p_event); event != nullptr)
		{
			if (isConsumed == false)
			{
				_enqueuePlatformAction(PlatformAction{
					.type = PlatformActionType::ValidateClosure});
			}
			else
			{
				_closureRequested.store(false);
			}
			return;
		}

		if (const auto *event = spk::getIf<spk::WindowResizedRecord>(p_event); event != nullptr)
		{
			_enqueueRenderAction(RenderAction{
				.type = RenderActionType::NotifyResize,
				.rect = event->rect});
		}

		if (spk::holds<spk::WindowDestroyedRecord>(p_event))
		{
			_isClosed.store(true);
			_closureNotificationPending.store(true);
		}
		return;
	}

	void Window::_treatKeyboardEvent(spk::KeyboardEventRecord &p_event)
	{
		_keyboardModule.pushEvent(p_event);
	}

	void Window::_treatMouseEvent(spk::MouseEventRecord &p_event)
	{
		_mouseModule.pushEvent(p_event);
	}

	void Window::_treatEvent(const EventReference& p_eventReference)
	{
		switch (p_eventReference.family)
		{
		case EventFamily::Frame:
			_treatFrameEvent(_frameEventsToProcess[p_eventReference.index]);
			break;
		case EventFamily::Mouse:
			_treatMouseEvent(_mouseEventsToProcess[p_eventReference.index]);
			break;
		case EventFamily::Keyboard:
			_treatKeyboardEvent(_keyboardEventsToProcess[p_eventReference.index]);
			break;
		}
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

	void Window::_executePlatformAction(const PlatformAction &p_action)
	{
		switch (p_action.type)
		{
		case PlatformActionType::RequestClosure:
			_host.requestClosure();
			break;
		case PlatformActionType::ValidateClosure:
			_host.validateClosure();
			break;
		case PlatformActionType::SetTitle:
			if (p_action.title.has_value() == true)
			{
				_host.setTitle(p_action.title.value());
			}
			break;
		case PlatformActionType::ResizeFrame:
			if (p_action.rect.has_value() == true)
			{
				_host.resize(p_action.rect.value());
			}
			break;
		}
	}

	void Window::_triggerClosureNotificationIfPending()
	{
		if (_closureNotificationPending.exchange(false) == true)
		{
			_closureEventProvider.trigger(this);
		}
	}

	void Window::_releasePlatformResourcesIfReady()
	{
		if (_isClosed.load() == false || _renderResourcesReleased.load() == false)
		{
			return;
		}

		if (_platformResourcesReleased.exchange(true) == false)
		{
			_host.releaseFrame();
		}
	}

	void Window::_executePendingPlatformActionsIfOnPlatformThread()
	{
		if (_host.isPlatformThread() == true)
		{
			executePendingPlatformActions();
		}
	}

	void Window::_processPendingEvents()
	{
		_drainPendingEvents();

		for (const EventReference& eventReference : _eventsToProcess)
		{
			_treatEvent(eventReference);

			if (_isClosed.load() == true)
			{
				return;
			}
		}
	}

	void Window::_releaseRenderResources()
	{
		if (_renderResourcesReleased.exchange(true) == false)
		{
			_host.releaseRenderContext();
		}
	}

	void Window::_executeRenderAction(const RenderAction &p_action)
	{
		switch (p_action.type)
		{
		case RenderActionType::NotifyResize:
			if (p_action.rect.has_value() == true)
			{
				_host.notifyFrameResized(p_action.rect.value());
			}
			break;
		case RenderActionType::SetVSync:
			if (p_action.vSyncEnabled.has_value() == true)
			{
				_host.setVSync(p_action.vSyncEnabled.value());
			}
			break;
		}
	}

	Window::Window(std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, Configuration p_configuration) :
		_rootWidget(":/" + p_configuration.title + "/RootWidget", nullptr),
		_host(_createFrame(p_platformRuntime, p_configuration.rect, p_configuration.title), std::move(p_gpuPlatformRuntime)), _renderSnapshot(std::make_shared<spk::RenderCommandBuilder>())
	{
		_rootWidget.setGeometry(p_configuration.rect.atOrigin());
		_rootWidget.activate();

		_frameModule.bind(&_rootWidget);
		_mouseModule.bind(&_rootWidget);
		_keyboardModule.bind(&_rootWidget);
		_updateModule.bind(&_rootWidget);
		_updateModule.bindInputs(&_mouseModule.mouse(), &_keyboardModule.keyboard());

		_frameEventQueueContract = _host.subscribeToFrameEvents(
			[this](const spk::FrameEventRecord &p_event)
			{
				_enqueueFrameEvent(p_event);
			});

		_mouseEventQueueContract = _host.subscribeToMouseEvents(
			[this](const spk::MouseEventRecord &p_event)
			{
				_enqueueMouseEvent(p_event);
			});

		_keyboardEventQueueContract = _host.subscribeToKeyboardEvents(
			[this](const spk::KeyboardEventRecord &p_event)
			{
				_enqueueKeyboardEvent(p_event);
			});

		_rebuildRenderSnapshot();
	}

	spk::WindowHost &Window::host()
	{
		return _host;
	}

	const spk::WindowHost &Window::host() const
	{
		return _host;
	}

	spk::Mouse &Window::mouse()
	{
		return _mouseModule.mouse();
	}

	const spk::Mouse &Window::mouse() const
	{
		return _mouseModule.mouse();
	}

	spk::Keyboard &Window::keyboard()
	{
		return _keyboardModule.keyboard();
	}

	const spk::Keyboard &Window::keyboard() const
	{
		return _keyboardModule.keyboard();
	}

	spk::Widget &Window::rootWidget()
	{
		return _rootWidget;
	}

	const spk::Widget &Window::rootWidget() const
	{
		return _rootWidget;
	}

	void Window::executePendingPlatformActions()
	{
		std::vector<PlatformAction> actions = _drainPendingPlatformActions();

		for (const PlatformAction &action : actions)
		{
			_executePlatformAction(action);
		}

		_triggerClosureNotificationIfPending();
		_releasePlatformResourcesIfReady();
	}

	void Window::update()
	{
		if (_isClosed.load() == true)
		{
			return;
		}

		_processPendingEvents();

		if (_isClosed.load() == false)
		{
			_updateModule.update();
			_rebuildRenderSnapshot();
		}
	}

	void Window::render()
	{
		if (_isClosed.load() == true)
		{
			_releaseRenderResources();
			return;
		}

		std::shared_ptr<spk::RenderCommandBuilder> snapshot = _currentRenderSnapshot();
		std::vector<RenderAction> renderActions = _drainPendingRenderActions();

		if (_host.makeCurrent() == false)
		{
			return;
		}

		for (const RenderAction &action : renderActions)
		{
			_executeRenderAction(action);
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
			.type = PlatformActionType::RequestClosure});

		_executePendingPlatformActionsIfOnPlatformThread();
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
