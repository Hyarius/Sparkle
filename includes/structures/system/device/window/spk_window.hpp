#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "structures/application/module/spk_frame_module.hpp"
#include "structures/application/module/spk_keyboard_module.hpp"
#include "structures/application/module/spk_mouse_module.hpp"
#include "structures/application/module/spk_render_module.hpp"
#include "structures/application/module/spk_update_module.hpp"
#include "structures/design_pattern/spk_contract_provider.hpp"
#include "structures/system/device/runtime/spk_platform_runtime.hpp"
#include "structures/system/device/window/spk_window_host.hpp"
#include "structures/system/device/window/spk_window_snapshot_manager.hpp"
#include "structures/system/spk_profiler.hpp"
#include "structures/system/thread/spk_thread_safe_contract.hpp"
#include "structures/system/thread/spk_thread_safe_deque.hpp"

namespace spk
{
	class WindowHandle;
	class Window
	{
	public:
		using ClosureEventProvider = spk::ContractProvider<Window *>;
		using ClosureEventProviderMutex = std::recursive_mutex;
		using ClosureContract = spk::ThreadSafeContract<ClosureEventProvider::Contract, ClosureEventProviderMutex>;
		using ClosureCallback = ClosureEventProvider::Callback;

		enum class PlatformActionType
		{
			RequestClosure,
			ValidateClosure,
			SetTitle,
			ResizeFrame,
			SetCursor
		};

		struct PlatformAction
		{
			PlatformActionType type;
			std::optional<spk::Rect2D> rect = std::nullopt;
			std::optional<std::string> title = std::nullopt;
			std::optional<std::string> cursorShape = std::nullopt;
		};

		enum class RenderActionType
		{
			NotifyResize,
			SetVSync
		};

		struct RenderAction
		{
			RenderActionType type;
			std::optional<spk::Rect2D> rect = std::nullopt;
			std::optional<bool> vSyncEnabled = std::nullopt;
		};

		struct Configuration
		{
			spk::Rect2D rect;
			std::string title;
		};

	private:
		spk::Widget _rootWidget;
		spk::WindowHost _host;
		spk::Profiler _profiler;

		FrameModule _frameModule;
		MouseModule _mouseModule;
		KeyboardModule _keyboardModule;
		UpdateModule _updateModule;
		RenderModule _renderModule;
		WindowSnapshotManager _snapshotManager;

		spk::ThreadSafeDeque<PlatformAction> _pendingPlatformActions;
		spk::ThreadSafeDeque<RenderAction> _pendingRenderActions;

		std::string _appliedCursorShape = "Arrow";

		std::atomic<bool> _isClosed = false;
		std::atomic<bool> _closureRequested = false;
		std::atomic<bool> _closureNotificationPending = false;
		std::atomic<bool> _renderResourcesReleased = false;
		std::atomic<bool> _platformResourcesReleased = false;

		spk::IFrame::FrameEventContract _frameEventQueueContract;
		spk::IFrame::MouseEventContract _mouseEventQueueContract;
		spk::IFrame::KeyboardEventContract _keyboardEventQueueContract;

		std::shared_ptr<ClosureEventProviderMutex> _closureEventProviderMutex = std::make_shared<ClosureEventProviderMutex>();
		ClosureEventProvider _closureEventProvider;

	private:
		void _enqueueFrameEvent(spk::FrameEventRecord p_event);
		void _enqueueMouseEvent(spk::MouseEventRecord p_event);
		void _enqueueKeyboardEvent(spk::KeyboardEventRecord p_event);
		void _enqueuePlatformAction(PlatformAction p_action);
		void _enqueueRenderAction(RenderAction p_action);

		[[nodiscard]] std::vector<PlatformAction> _drainPendingPlatformActions();
		[[nodiscard]] std::vector<RenderAction> _drainPendingRenderActions();

		void _treatProcessedFrameEvent(spk::FrameEventRecord &p_event, bool p_isConsumed);

		void _rebuildRenderSnapshot();

		void _executePlatformAction(const PlatformAction &p_action);
		void _triggerClosureNotificationIfPending();
		void _releasePlatformResourcesIfReady();

		void _processPendingFrameEvents();
		void _processPendingMouseEvents();
		void _processPendingKeyboardEvents();
		void _processPendingEvents();

		void _releaseRenderResources();
		void _executeRenderAction(const RenderAction &p_action);

		void _setTitle(std::string p_title);
		void _resize(const spk::Rect2D &p_rect);
		void _setVSync(bool p_enabled);
		[[nodiscard]] spk::Widget &_centralWidget();

		friend class spk::WindowHandle;

	public:
		Window(std::shared_ptr<PlatformRuntime> p_platformRuntime, Configuration p_configuration);
		~Window();

		[[nodiscard]] spk::WindowHost &host();
		[[nodiscard]] const spk::WindowHost &host() const;

		[[nodiscard]] spk::Mouse &mouse();
		[[nodiscard]] const spk::Mouse &mouse() const;

		[[nodiscard]] spk::Keyboard &keyboard();
		[[nodiscard]] const spk::Keyboard &keyboard() const;

		[[nodiscard]] spk::Widget &rootWidget();
		[[nodiscard]] const spk::Widget &rootWidget() const;

		void executePendingPlatformActions();
		void update();
		void render();
		void requestClosure();
		[[nodiscard]] bool isClosed() const;
		[[nodiscard]] bool isReadyForDisposal() const;

		ClosureContract subscribeToClosure(ClosureCallback p_callback);
	};
}
