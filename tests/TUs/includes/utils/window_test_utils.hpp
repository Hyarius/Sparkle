#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "spk_application.hpp"
#include "spk_events.hpp"
#include "spk_frame.hpp"
#include "spk_gpu_platform_runtime.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_render_command.hpp"
#include "spk_render_context.hpp"
#include "spk_update_tick.hpp"
#include "spk_widget.hpp"
#include "spk_window.hpp"
#include "spk_window_host.hpp"

namespace sparkle_test
{
	class WindowAccess
	{
	public:
		[[nodiscard]] static spk::WindowHost& host(spk::Window& p_window)
		{
			return p_window.host();
		}

		[[nodiscard]] static const spk::WindowHost& host(const spk::Window& p_window)
		{
			return p_window.host();
		}

		[[nodiscard]] static spk::Mouse& mouse(spk::Window& p_window)
		{
			return p_window.mouse();
		}

		[[nodiscard]] static const spk::Mouse& mouse(const spk::Window& p_window)
		{
			return p_window.mouse();
		}

		[[nodiscard]] static spk::Keyboard& keyboard(spk::Window& p_window)
		{
			return p_window.keyboard();
		}

		[[nodiscard]] static const spk::Keyboard& keyboard(const spk::Window& p_window)
		{
			return p_window.keyboard();
		}

		[[nodiscard]] static spk::Widget& rootWidget(spk::Window& p_window)
		{
			return p_window.rootWidget();
		}

		[[nodiscard]] static const spk::Widget& rootWidget(const spk::Window& p_window)
		{
			return p_window.rootWidget();
		}
	};

	inline spk::Rect2D defaultRect()
	{
		return spk::Rect2D(10, 20, 300, 400);
	}

	template <typename TEventData>
	inline std::string eventKind()
	{
		if constexpr (std::is_same_v<TEventData, spk::WindowCloseRequestedRecord>) return "WindowCloseRequested";
		if constexpr (std::is_same_v<TEventData, spk::WindowDestroyedRecord>) return "WindowDestroyed";
		if constexpr (std::is_same_v<TEventData, spk::WindowMovedRecord>) return "WindowMoved";
		if constexpr (std::is_same_v<TEventData, spk::WindowResizedRecord>) return "WindowResized";
		if constexpr (std::is_same_v<TEventData, spk::WindowFocusGainedRecord>) return "WindowFocusGained";
		if constexpr (std::is_same_v<TEventData, spk::WindowFocusLostRecord>) return "WindowFocusLost";
		if constexpr (std::is_same_v<TEventData, spk::WindowShownRecord>) return "WindowShown";
		if constexpr (std::is_same_v<TEventData, spk::WindowHiddenRecord>) return "WindowHidden";
		if constexpr (std::is_same_v<TEventData, spk::MouseEnteredRecord>) return "MouseEntered";
		if constexpr (std::is_same_v<TEventData, spk::MouseLeftRecord>) return "MouseLeft";
		if constexpr (std::is_same_v<TEventData, spk::MouseMovedRecord>) return "MouseMoved";
		if constexpr (std::is_same_v<TEventData, spk::MouseWheelScrolledRecord>) return "MouseWheelScrolled";
		if constexpr (std::is_same_v<TEventData, spk::MouseButtonPressedRecord>) return "MouseButtonPressed";
		if constexpr (std::is_same_v<TEventData, spk::MouseButtonReleasedRecord>) return "MouseButtonReleased";
		if constexpr (std::is_same_v<TEventData, spk::MouseButtonDoubleClickedRecord>) return "MouseButtonDoubleClicked";
		if constexpr (std::is_same_v<TEventData, spk::KeyPressedRecord>) return "KeyPressed";
		if constexpr (std::is_same_v<TEventData, spk::KeyReleasedRecord>) return "KeyReleased";
		if constexpr (std::is_same_v<TEventData, spk::TextInputRecord>) return "TextInput";

		return "Unknown";
	}

	template <typename TEventData>
	inline std::string eventKind(const spk::EventView<TEventData>&)
	{
		return eventKind<TEventData>();
	}

	class CallbackRenderCommand : public spk::RenderCommand
	{
	private:
		std::function<void(spk::IRenderContext&)> _callback = nullptr;

	public:
		explicit CallbackRenderCommand(std::function<void()> p_callback) :
			_callback(
				[p_callback = std::move(p_callback)](spk::IRenderContext&)
				{
					if (p_callback != nullptr)
					{
						p_callback();
					}
				})
		{
		}

		explicit CallbackRenderCommand(std::function<void(spk::IRenderContext&)> p_callback) :
			_callback(std::move(p_callback))
		{
		}

		void execute(spk::IRenderContext& p_renderContext) override
		{
			if (_callback != nullptr)
			{
				_callback(p_renderContext);
			}
		}
	};

	class TestSurfaceState : public spk::ISurfaceState
	{

	};

	struct TestFrameStats
	{
		spk::Rect2D currentRect;
		std::string currentTitle;
		int resizeCount = 0;
		int setTitleCount = 0;
		int requestClosureCount = 0;
		int validateClosureCount = 0;
		std::vector<spk::Rect2D> resizeHistory;
		std::vector<std::string> titleHistory;
		std::thread::id lastResizeThreadID;
		std::thread::id lastSetTitleThreadID;
		std::thread::id lastRequestClosureThreadID;
		std::thread::id lastValidateClosureThreadID;
		bool destroyed = false;
	};

	class TestFrame : public spk::IFrame
	{
	private:
		std::shared_ptr<TestFrameStats> _stats;

	public:
		spk::Rect2D currentRect;
		std::string currentTitle;
		int resizeCount = 0;
		int setTitleCount = 0;
		int requestClosureCount = 0;
		int validateClosureCount = 0;
		std::vector<spk::Rect2D> resizeHistory;
		std::vector<std::string> titleHistory;
		std::thread::id lastResizeThreadID;
		std::thread::id lastSetTitleThreadID;
		std::thread::id lastRequestClosureThreadID;
		std::thread::id lastValidateClosureThreadID;

	public:
		TestFrame(const spk::Rect2D& p_rect, std::string p_title, std::shared_ptr<TestFrameStats> p_stats = std::make_shared<TestFrameStats>()) :
			spk::IFrame(std::make_shared<TestSurfaceState>()),
			_stats(std::move(p_stats)),
			currentRect(p_rect),
			currentTitle(std::move(p_title))
		{
			if (_stats != nullptr)
			{
				_stats->currentRect = currentRect;
				_stats->currentTitle = currentTitle;
			}
		}

		~TestFrame() override
		{
			if (_stats != nullptr)
			{
				_stats->destroyed = true;
			}
		}

		void resize(const spk::Rect2D& p_rect) override
		{
			currentRect = p_rect;
			++resizeCount;
			resizeHistory.push_back(p_rect);
			lastResizeThreadID = std::this_thread::get_id();

			if (_stats != nullptr)
			{
				_stats->currentRect = currentRect;
				++_stats->resizeCount;
				_stats->resizeHistory.push_back(p_rect);
				_stats->lastResizeThreadID = lastResizeThreadID;
			}
		}

		void setTitle(const std::string& p_title) override
		{
			currentTitle = p_title;
			++setTitleCount;
			titleHistory.push_back(p_title);
			lastSetTitleThreadID = std::this_thread::get_id();

			if (_stats != nullptr)
			{
				_stats->currentTitle = currentTitle;
				++_stats->setTitleCount;
				_stats->titleHistory.push_back(p_title);
				_stats->lastSetTitleThreadID = lastSetTitleThreadID;
			}
		}

		void requestClosure() override
		{
			++requestClosureCount;
			lastRequestClosureThreadID = std::this_thread::get_id();

			if (_stats != nullptr)
			{
				++_stats->requestClosureCount;
				_stats->lastRequestClosureThreadID = lastRequestClosureThreadID;
			}
		}

		void validateClosure() override
		{
			++validateClosureCount;
			lastValidateClosureThreadID = std::this_thread::get_id();

			if (_stats != nullptr)
			{
				++_stats->validateClosureCount;
				_stats->lastValidateClosureThreadID = lastValidateClosureThreadID;
			}
		}

		[[nodiscard]] spk::Rect2D rect() const override
		{
			return currentRect;
		}

		[[nodiscard]] std::string title() const override
		{
			return currentTitle;
		}

		void emitMouseEvent(const spk::MouseEventRecord& p_event)
		{
			_emitMouseEvent(p_event);
		}

		void emitKeyboardEvent(const spk::KeyboardEventRecord& p_event)
		{
			_emitKeyboardEvent(p_event);
		}

		void emitFrameEvent(const spk::FrameEventRecord& p_event)
		{
			_emitFrameEvent(p_event);
		}
	};

	class TestPlatformRuntime : public spk::IPlatformRuntime
	{
	private:
		enum class PendingEventKind
		{
			Mouse,
			Keyboard,
			Frame
		};

		struct PendingEvent
		{
			PendingEventKind kind;
			std::variant<spk::MouseEventRecord, spk::KeyboardEventRecord, spk::FrameEventRecord> event;
		};

		std::vector<PendingEvent> _pendingEvents;

		TestFrame& _frame()
		{
			if (createdFrame == nullptr)
			{
				throw std::runtime_error("TestPlatformRuntime requires a created frame before dispatching events");
			}

			return *createdFrame;
		}

	public:
		int createFrameCount = 0;
		int pollEventsCount = 0;
		bool returnNullFrame = false;
		std::function<void(TestPlatformRuntime&)> onPollEvents = nullptr;
		std::shared_ptr<TestFrameStats> frameStats = std::make_shared<TestFrameStats>();
		spk::Rect2D lastCreateRect;
		std::string lastCreateTitle;
		TestFrame* createdFrame = nullptr;

	public:
		std::unique_ptr<spk::IFrame> createFrame(const spk::Rect2D& p_rect, const std::string& p_title) override
		{
			++createFrameCount;
			lastCreateRect = p_rect;
			lastCreateTitle = p_title;

			if (returnNullFrame == true)
			{
				createdFrame = nullptr;
				frameStats = std::make_shared<TestFrameStats>();
				return nullptr;
			}

			frameStats = std::make_shared<TestFrameStats>();
			auto result = std::make_unique<TestFrame>(p_rect, p_title, frameStats);
			createdFrame = result.get();
			return result;
		}

		void pollEvents() override
		{
			++pollEventsCount;

			auto events = std::move(_pendingEvents);
			_pendingEvents.clear();

			for (auto& pendingEvent : events)
			{
				switch (pendingEvent.kind)
				{
				case PendingEventKind::Mouse:
					_frame().emitMouseEvent(std::get<spk::MouseEventRecord>(pendingEvent.event));
					break;
				case PendingEventKind::Keyboard:
					_frame().emitKeyboardEvent(std::get<spk::KeyboardEventRecord>(pendingEvent.event));
					break;
				case PendingEventKind::Frame:
					_frame().emitFrameEvent(std::get<spk::FrameEventRecord>(pendingEvent.event));
					break;
				}
			}

			if (onPollEvents != nullptr)
			{
				onPollEvents(*this);
			}
		}

		template <typename TEventData>
		void emitMouseEventRecord(TEventData p_data)
		{
			_frame().emitMouseEvent(spk::MouseEventRecord(spk::makeEventRecord(std::move(p_data))));
		}

		template <typename TEventData>
		void emitKeyboardEventRecord(TEventData p_data)
		{
			_frame().emitKeyboardEvent(spk::KeyboardEventRecord(spk::makeEventRecord(std::move(p_data))));
		}

		template <typename TEventData>
		void emitFrameEventRecord(TEventData p_data)
		{
			_frame().emitFrameEvent(spk::FrameEventRecord(spk::makeEventRecord(std::move(p_data))));
		}

		template <typename TEventData>
		void queueMouseEvent(TEventData p_data)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Mouse,
				.event = spk::MouseEventRecord(spk::makeEventRecord(std::move(p_data)))
			});
		}

		template <typename TEventData>
		void queueKeyboardEvent(TEventData p_data)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Keyboard,
				.event = spk::KeyboardEventRecord(spk::makeEventRecord(std::move(p_data)))
			});
		}

		template <typename TEventData>
		void queueFrameEvent(TEventData p_data)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Frame,
				.event = spk::FrameEventRecord(spk::makeEventRecord(std::move(p_data)))
			});
		}
	};

	struct TestRenderContextStats
	{
		int invalidateCount = 0;
		int makeCurrentCount = 0;
		int presentCount = 0;
		int setVSyncCount = 0;
		int notifyResizeCount = 0;
		bool lastVSync = false;
		spk::Rect2D lastResizeRect;
		std::vector<spk::Rect2D> resizeHistory;
		std::thread::id creationThreadID;
		std::thread::id lastMakeCurrentThreadID;
		std::thread::id lastPresentThreadID;
		std::thread::id lastSetVSyncThreadID;
		std::thread::id lastNotifyResizeThreadID;
		std::thread::id lastInvalidateThreadID;
		bool destroyed = false;
	};

	class TestRenderContext : public spk::IRenderContext
	{
	private:
		std::shared_ptr<TestRenderContextStats> _stats;

	public:
		int invalidateCount = 0;
		int makeCurrentCount = 0;
		int presentCount = 0;
		int setVSyncCount = 0;
		int notifyResizeCount = 0;
		bool lastVSync = false;
		spk::Rect2D lastResizeRect;
		std::vector<spk::Rect2D> resizeHistory;
		std::thread::id creationThreadID;
		std::thread::id lastMakeCurrentThreadID;
		std::thread::id lastPresentThreadID;
		std::thread::id lastSetVSyncThreadID;
		std::thread::id lastNotifyResizeThreadID;
		std::thread::id lastInvalidateThreadID;

	public:
		explicit TestRenderContext(std::shared_ptr<spk::ISurfaceState> p_surfaceState, std::shared_ptr<TestRenderContextStats> p_stats = std::make_shared<TestRenderContextStats>()) :
			spk::IRenderContext(std::move(p_surfaceState)),
			_stats(std::move(p_stats))
		{
		}

		~TestRenderContext() override
		{
			if (_stats != nullptr)
			{
				_stats->destroyed = true;
			}
		}

		void invalidate() override
		{
			++invalidateCount;
			lastInvalidateThreadID = std::this_thread::get_id();
			if (_stats != nullptr)
			{
				++_stats->invalidateCount;
				_stats->lastInvalidateThreadID = lastInvalidateThreadID;
			}
			spk::IRenderContext::invalidate();
		}

		[[nodiscard]] bool isValid() const override
		{
			return spk::IRenderContext::isValid();
		}

		void makeCurrent() override
		{
			++makeCurrentCount;
			lastMakeCurrentThreadID = std::this_thread::get_id();
			if (_stats != nullptr)
			{
				++_stats->makeCurrentCount;
				_stats->lastMakeCurrentThreadID = lastMakeCurrentThreadID;
			}
		}

		void present() override
		{
			++presentCount;
			lastPresentThreadID = std::this_thread::get_id();
			if (_stats != nullptr)
			{
				++_stats->presentCount;
				_stats->lastPresentThreadID = lastPresentThreadID;
			}
		}

		void setVSync(bool p_enabled) override
		{
			++setVSyncCount;
			lastVSync = p_enabled;
			lastSetVSyncThreadID = std::this_thread::get_id();
			if (_stats != nullptr)
			{
				++_stats->setVSyncCount;
				_stats->lastVSync = p_enabled;
				_stats->lastSetVSyncThreadID = lastSetVSyncThreadID;
			}
		}

		void notifyResize(const spk::Rect2D& p_rect) override
		{
			++notifyResizeCount;
			lastResizeRect = p_rect;
			resizeHistory.push_back(p_rect);
			lastNotifyResizeThreadID = std::this_thread::get_id();
			if (_stats != nullptr)
			{
				++_stats->notifyResizeCount;
				_stats->lastResizeRect = lastResizeRect;
				_stats->resizeHistory.push_back(p_rect);
				_stats->lastNotifyResizeThreadID = lastNotifyResizeThreadID;
			}
		}
	};

	class TestGPUPlatformRuntime : public spk::IGPUPlatformRuntime
	{
	public:
		int createRenderContextCount = 0;
		bool returnNullContext = false;
		std::shared_ptr<TestRenderContextStats> contextStats = std::make_shared<TestRenderContextStats>();
		spk::IFrame* lastFrame = nullptr;
		TestRenderContext* createdContext = nullptr;
		std::thread::id lastCreateRenderContextThreadID;

	public:
		std::unique_ptr<spk::IRenderContext> createRenderContext(spk::IFrame& p_frame) override
		{
			++createRenderContextCount;
			lastFrame = &p_frame;
			lastCreateRenderContextThreadID = std::this_thread::get_id();

			if (returnNullContext == true)
			{
				createdContext = nullptr;
				contextStats = std::make_shared<TestRenderContextStats>();
				return nullptr;
			}

			contextStats = std::make_shared<TestRenderContextStats>();
			auto result = std::make_unique<TestRenderContext>(p_frame.surfaceState(), contextStats);
			result->creationThreadID = std::this_thread::get_id();
			contextStats->creationThreadID = result->creationThreadID;
			createdContext = result.get();
			return result;
		}
	};

	class RecordingWidget : public spk::Widget
	{
	public:
		using spk::Widget::Widget;

		mutable int geometryUpdateCount = 0;
		mutable int appendRenderCommandCount = 0;
		mutable int renderCount = 0;
		int updateCount = 0;
		int frameEventCount = 0;
		int mouseEventCount = 0;
		int keyboardEventCount = 0;

		bool consumeFrameEvent = false;
		bool consumeMouseEvent = false;
		bool consumeKeyboardEvent = false;

		mutable spk::Rect2D geometrySeenDuringUpdate;
		const spk::Mouse* lastTickMouse = nullptr;
		const spk::Keyboard* lastTickKeyboard = nullptr;
		std::vector<std::string>* sharedCallLog = nullptr;
		mutable std::vector<std::string> callLog;
		std::vector<std::string> frameEventKinds;
		std::vector<std::string> mouseEventKinds;
		std::vector<std::string> keyboardEventKinds;

	public:
		void bindSharedCallLog(std::vector<std::string>* p_log)
		{
			sharedCallLog = p_log;
		}

	protected:
		spk::RenderUnit _buildRenderUnit() const override
		{
			if (isRenderCommandDirty() == true)
			{
				++geometryUpdateCount;
				geometrySeenDuringUpdate = geometry();
			}

			++appendRenderCommandCount;
			callLog.push_back(name() + ":append_render");
			if (sharedCallLog != nullptr)
			{
				sharedCallLog->push_back(name() + ":append_render");
			}

			spk::RenderUnitBuilder builder;
			builder.emplace<CallbackRenderCommand>(
				[this]()
				{
					++renderCount;
					callLog.push_back(name() + ":render");
					if (sharedCallLog != nullptr)
					{
						sharedCallLog->push_back(name() + ":render");
					}
				});

			return builder.build();
		}

		void _onUpdate(const spk::UpdateTick& p_tick) override
		{
			++updateCount;
			lastTickMouse = p_tick.mouse;
			lastTickKeyboard = p_tick.keyboard;
			callLog.push_back(name() + ":update");
			if (sharedCallLog != nullptr)
			{
				sharedCallLog->push_back(name() + ":update");
			}
		}

		template <typename TEventData>
		void _recordFrameEvent(spk::EventView<TEventData>& p_event)
		{
			++frameEventCount;
			frameEventKinds.push_back(eventKind(p_event));
			callLog.push_back(name() + ":frame:" + eventKind(p_event));
			if (sharedCallLog != nullptr)
			{
				sharedCallLog->push_back(name() + ":frame:" + eventKind(p_event));
			}

			if (consumeFrameEvent == true)
			{
				p_event.consume();
			}
		}

		template <typename TEventData>
		void _recordMouseEvent(spk::EventView<TEventData>& p_event)
		{
			++mouseEventCount;
			mouseEventKinds.push_back(eventKind(p_event));
			callLog.push_back(name() + ":mouse:" + eventKind(p_event));
			if (sharedCallLog != nullptr)
			{
				sharedCallLog->push_back(name() + ":mouse:" + eventKind(p_event));
			}

			if (consumeMouseEvent == true)
			{
				p_event.consume();
			}
		}

		template <typename TEventData>
		void _recordKeyboardEvent(spk::EventView<TEventData>& p_event)
		{
			++keyboardEventCount;
			keyboardEventKinds.push_back(eventKind(p_event));
			callLog.push_back(name() + ":keyboard:" + eventKind(p_event));
			if (sharedCallLog != nullptr)
			{
				sharedCallLog->push_back(name() + ":keyboard:" + eventKind(p_event));
			}

			if (consumeKeyboardEvent == true)
			{
				p_event.consume();
			}
		}

		void _onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowDestroyedEvent(spk::WindowDestroyedEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowMovedEvent(spk::WindowMovedEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowResizedEvent(spk::WindowResizedEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowFocusGainedEvent(spk::WindowFocusGainedEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowFocusLostEvent(spk::WindowFocusLostEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowShownEvent(spk::WindowShownEvent& p_event) override { _recordFrameEvent(p_event); }
		void _onWindowHiddenEvent(spk::WindowHiddenEvent& p_event) override { _recordFrameEvent(p_event); }

		void _onMouseEnteredEvent(spk::MouseEnteredWindowEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseLeftEvent(spk::MouseLeftWindowEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseMovedEvent(spk::MouseMovedEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent& p_event) override { _recordMouseEvent(p_event); }
		void _onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent& p_event) override { _recordMouseEvent(p_event); }

		void _onKeyPressedEvent(spk::KeyPressedEvent& p_event) override { _recordKeyboardEvent(p_event); }
		void _onKeyReleasedEvent(spk::KeyReleasedEvent& p_event) override { _recordKeyboardEvent(p_event); }
		void _onTextInputEvent(spk::TextInputEvent& p_event) override { _recordKeyboardEvent(p_event); }
	};

	class CallbackWidget : public spk::Widget
	{
	public:
		using spk::Widget::Widget;

		std::function<void(const spk::UpdateTick&)> onUpdate = nullptr;
		std::function<void()> onRender = nullptr;
		std::function<void()> onAppendRenderCommands = nullptr;
		std::function<void()> onExecuteRenderCommand = nullptr;

	protected:
		void _onUpdate(const spk::UpdateTick& p_tick) override
		{
			if (onUpdate != nullptr)
			{
				onUpdate(p_tick);
			}
		}

		spk::RenderUnit _buildRenderUnit() const override
		{
			if (onAppendRenderCommands != nullptr)
			{
				onAppendRenderCommands();
			}

			spk::RenderUnitBuilder builder;
			if (onExecuteRenderCommand != nullptr)
			{
				builder.emplace<CallbackRenderCommand>(onExecuteRenderCommand);
			}
			else if (onRender != nullptr)
			{
				builder.emplace<CallbackRenderCommand>(onRender);
			}

			return builder.build();
		}
	};

	struct WindowHostBundle
	{
		std::shared_ptr<TestPlatformRuntime> platformRuntime = nullptr;
		std::shared_ptr<TestGPUPlatformRuntime> gpuPlatformRuntime = nullptr;
		std::unique_ptr<spk::WindowHost> windowHost = nullptr;
	};

	inline WindowHostBundle createWindowHostBundle(const spk::Rect2D& p_rect = defaultRect(), const std::string& p_title = "TestWindow")
	{
		auto platformRuntime = std::make_shared<TestPlatformRuntime>();
		auto gpuPlatformRuntime = std::make_shared<TestGPUPlatformRuntime>();

		WindowHostBundle result;
		result.platformRuntime = platformRuntime;
		result.gpuPlatformRuntime = gpuPlatformRuntime;
		result.windowHost = std::make_unique<spk::WindowHost>(
			platformRuntime->createFrame(p_rect, p_title),
			std::move(gpuPlatformRuntime));

		return result;
	}

	struct WindowBundle
	{
		std::shared_ptr<TestPlatformRuntime> platformRuntime = nullptr;
		std::shared_ptr<TestGPUPlatformRuntime> gpuPlatformRuntime = nullptr;
		std::unique_ptr<spk::Window> window = nullptr;
	};

	inline WindowBundle createWindowBundle(const spk::Rect2D& p_rect = defaultRect(), const std::string& p_title = "Window")
	{
		auto platformRuntime = std::make_shared<TestPlatformRuntime>();
		auto gpuPlatformRuntime = std::make_shared<TestGPUPlatformRuntime>();

		WindowBundle result;
		result.platformRuntime = platformRuntime;
		result.gpuPlatformRuntime = gpuPlatformRuntime;
		result.window = std::make_unique<spk::Window>(
			platformRuntime,
			gpuPlatformRuntime,
			spk::Window::Configuration{
				.rect = p_rect,
				.title = p_title
			});

		return result;
	}
}
