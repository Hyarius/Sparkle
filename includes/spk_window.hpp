#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "spk_contract_provider.hpp"
#include "spk_gpu_platform_runtime.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_render_command_builder.hpp"
#include "spk_window_host.hpp"
#include "spk_window_modules.hpp"

namespace spk
{
	class Window
	{
	public:
		using ClosureEventProvider = spk::ContractProvider<Window*>;
		using ClosureContract = ClosureEventProvider::Contract;
		using ClosureCallback = ClosureEventProvider::Callback;

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

		mutable std::mutex _pendingEventsMutex;
		std::vector<spk::Event> _pendingEvents;

		mutable std::mutex _pendingFrameResizeMutex;
		std::optional<spk::Rect2D> _pendingFrameResize;

		mutable std::mutex _renderSnapshotMutex;
		std::shared_ptr<spk::RenderCommandBuilder> _renderSnapshot;

		spk::IFrame::EventContract _frameEventQueueContract;
		spk::IFrame::EventContract _mouseEventQueueContract;
		spk::IFrame::EventContract _keyboardEventQueueContract;

		ClosureEventProvider _closureEventProvider;

	private:
		void _enqueueEvent(const spk::Event& p_event);
		void _recordPendingFrameResize(const spk::Rect2D& p_rect);

		[[nodiscard]] std::vector<spk::Event> _drainPendingEvents();
		[[nodiscard]] std::optional<spk::Rect2D> _consumePendingFrameResize();

		void _treatEvent(const spk::Event& p_event);

		void _rebuildRenderSnapshot();
		[[nodiscard]] std::shared_ptr<spk::RenderCommandBuilder> _currentRenderSnapshot() const;

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

		void update();
		void render();
		void requestClosure();

		ClosureContract subscribeToClosure(ClosureCallback p_callback);
	};
}
