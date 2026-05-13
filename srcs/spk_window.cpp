#include "spk_window.hpp"

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
		_frameModule.pushEvent(std::move(p_event));
	}

	void Window::_enqueueMouseEvent(spk::MouseEventRecord p_event)
	{
		_mouseModule.pushEvent(std::move(p_event));
	}

	void Window::_enqueueKeyboardEvent(spk::KeyboardEventRecord p_event)
	{
		_keyboardModule.pushEvent(std::move(p_event));
	}

	void Window::_enqueuePlatformAction(PlatformAction p_action)
	{
		_pendingPlatformActions.pushBack(std::move(p_action));
	}

	void Window::_enqueueRenderAction(RenderAction p_action)
	{
		_pendingRenderActions.pushBack(std::move(p_action));
	}

	std::vector<Window::PlatformAction> Window::_drainPendingPlatformActions()
	{
		return _pendingPlatformActions.drain();
	}

	std::vector<Window::RenderAction> Window::_drainPendingRenderActions()
	{
		std::vector<RenderAction> result = _pendingRenderActions.drain();

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

	void Window::_treatProcessedFrameEvent(spk::FrameEventRecord &p_event, bool p_isConsumed)
	{
		if (const auto* event = spk::getIf<spk::WindowCloseRequestedRecord>(p_event); event != nullptr)
		{
			if (p_isConsumed == false)
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

	void Window::_processPendingFrameEvents()
	{
		_frameModule.processEvents(
			[this](spk::FrameEventRecord& p_event, bool p_isConsumed) -> bool
			{
				if (_isClosed.load() == true)
				{
					return false;
				}

				_treatProcessedFrameEvent(p_event, p_isConsumed);
				return (_isClosed.load() == false);
			});
	}

	void Window::_processPendingMouseEvents()
	{
		_mouseModule.processEvents();
	}

	void Window::_processPendingKeyboardEvents()
	{
		_keyboardModule.processEvents();
	}

	void Window::_processPendingEvents()
	{
		_processPendingFrameEvents();

		if (_isClosed.load() == true)
		{
			return;
		}

		_processPendingMouseEvents();
		_processPendingKeyboardEvents();
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
