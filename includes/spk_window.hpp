#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "spk_contract_provider.hpp"
#include "spk_frame_module.hpp"
#include "spk_gpu_platform_runtime.hpp"
#include "spk_keyboard_module.hpp"
#include "spk_mouse_module.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_render_command_builder.hpp"
#include "spk_render_module.hpp"
#include "spk_thread_safe_deque.hpp"
#include "spk_update_module.hpp"
#include "spk_window_host.hpp"

namespace spk
{
	class Window
	{
	public:
		using ClosureEventProvider = spk::ContractProvider<Window*>;
		using ClosureContract = ClosureEventProvider::Contract;
		using ClosureCallback = ClosureEventProvider::Callback;

		enum class PlatformActionType
		{
			RequestClosure,
			ValidateClosure,
			SetTitle,
			ResizeFrame
		};

		struct PlatformAction
		{
			PlatformActionType type;
			std::optional<spk::Rect2D> rect = std::nullopt;
			std::optional<std::string> title = std::nullopt;
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

		FrameModule _frameModule;
		MouseModule _mouseModule;
		KeyboardModule _keyboardModule;
		UpdateModule _updateModule;
		RenderModule _renderModule;

		spk::ThreadSafeDeque<PlatformAction> _pendingPlatformActions;
		spk::ThreadSafeDeque<RenderAction> _pendingRenderActions;

		mutable std::mutex _renderSnapshotMutex;
		std::shared_ptr<spk::RenderCommandBuilder> _renderSnapshot;

		std::atomic<bool> _isClosed = false;
		std::atomic<bool> _closureRequested = false;
		std::atomic<bool> _closureNotificationPending = false;
		std::atomic<bool> _renderResourcesReleased = false;
		std::atomic<bool> _platformResourcesReleased = false;

		spk::IFrame::FrameEventContract _frameEventQueueContract;
		spk::IFrame::MouseEventContract _mouseEventQueueContract;
		spk::IFrame::KeyboardEventContract _keyboardEventQueueContract;

		ClosureEventProvider _closureEventProvider;

	private:
		void _enqueueFrameEvent(spk::FrameEventRecord p_event);
		void _enqueueMouseEvent(spk::MouseEventRecord p_event);
		void _enqueueKeyboardEvent(spk::KeyboardEventRecord p_event);
		void _enqueuePlatformAction(PlatformAction p_action);
		void _enqueueRenderAction(RenderAction p_action);

		[[nodiscard]] std::vector<PlatformAction> _drainPendingPlatformActions();
		[[nodiscard]] std::vector<RenderAction> _drainPendingRenderActions();

		void _treatProcessedFrameEvent(spk::FrameEventRecord& p_event, bool p_isConsumed);

		void _rebuildRenderSnapshot();
		[[nodiscard]] std::shared_ptr<spk::RenderCommandBuilder> _currentRenderSnapshot() const;

		void _executePlatformAction(const PlatformAction& p_action);
		void _triggerClosureNotificationIfPending();
		void _releasePlatformResourcesIfReady();
		void _executePendingPlatformActionsIfOnPlatformThread();

		void _processPendingFrameEvents();
		void _processPendingMouseEvents();
		void _processPendingKeyboardEvents();
		void _processPendingEvents();

		void _releaseRenderResources();
		void _executeRenderAction(const RenderAction& p_action);

	public:
		Window(std::shared_ptr<IPlatformRuntime> p_platformRuntime, std::shared_ptr<IGPUPlatformRuntime> p_gpuPlatformRuntime, Configuration p_configuration);

		[[nodiscard]] spk::WindowHost& host();
		[[nodiscard]] const spk::WindowHost& host() const;

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;

		[[nodiscard]] spk::Widget& rootWidget();
		[[nodiscard]] const spk::Widget& rootWidget() const;

		void executePendingPlatformActions();
		void update();
		void render();
		void requestClosure();
		[[nodiscard]] bool isClosed() const;
		[[nodiscard]] bool isReadyForDisposal() const;

		ClosureContract subscribeToClosure(ClosureCallback p_callback);
	};
}
