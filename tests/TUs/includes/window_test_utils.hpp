#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "spk_events.hpp"
#include "spk_frame.hpp"
#include "spk_platform_runtime.hpp"
#include "spk_render_command.hpp"
#include "spk_render_command_builder.hpp"
#include "spk_render_context.hpp"
#include "spk_update_tick.hpp"
#include "spk_widget.hpp"
#include "spk_window.hpp"
#include "spk_window_host.hpp"

namespace sparkle_test
{
	inline spk::Rect2D defaultRect()
	{
		return spk::Rect2D(10, 20, 300, 400);
	}

	inline std::string eventKind(const spk::Event& p_event)
	{
		if (p_event.holds<spk::WindowCloseRequestedPayload>()) return "WindowCloseRequested";
		if (p_event.holds<spk::WindowCloseValidatedPayload>()) return "WindowCloseValidated";
		if (p_event.holds<spk::WindowMovedPayload>()) return "WindowMoved";
		if (p_event.holds<spk::WindowResizedPayload>()) return "WindowResized";
		if (p_event.holds<spk::WindowFocusGainedPayload>()) return "WindowFocusGained";
		if (p_event.holds<spk::WindowFocusLostPayload>()) return "WindowFocusLost";
		if (p_event.holds<spk::WindowShownPayload>()) return "WindowShown";
		if (p_event.holds<spk::WindowHiddenPayload>()) return "WindowHidden";
		if (p_event.holds<spk::MouseEnteredPayload>()) return "MouseEntered";
		if (p_event.holds<spk::MouseLeftPayload>()) return "MouseLeft";
		if (p_event.holds<spk::MouseMovedPayload>()) return "MouseMoved";
		if (p_event.holds<spk::MouseWheelScrolledPayload>()) return "MouseWheelScrolled";
		if (p_event.holds<spk::MouseButtonPressedPayload>()) return "MouseButtonPressed";
		if (p_event.holds<spk::MouseButtonReleasedPayload>()) return "MouseButtonReleased";
		if (p_event.holds<spk::MouseButtonDoubleClickedPayload>()) return "MouseButtonDoubleClicked";
		if (p_event.holds<spk::KeyPressedPayload>()) return "KeyPressed";
		if (p_event.holds<spk::KeyReleasedPayload>()) return "KeyReleased";
		if (p_event.holds<spk::TextInputPayload>()) return "TextInput";

		return "Unknown";
	}

	class CallbackRenderCommand : public spk::RenderCommand
	{
	private:
		std::function<void()> _callback = nullptr;

	public:
		explicit CallbackRenderCommand(std::function<void()> p_callback) :
			_callback(std::move(p_callback))
		{
		}

		void execute() const override
		{
			if (_callback != nullptr)
			{
				_callback();
			}
		}
	};

	class TestFrame : public spk::IFrame
	{
	public:
		spk::Rect2D currentRect;
		std::string currentTitle;
		int resizeCount = 0;
		int setTitleCount = 0;
		int requestClosureCount = 0;
		int createRenderContextCount = 0;
		std::vector<spk::Rect2D> resizeHistory;
		std::vector<std::string> titleHistory;
		spk::IRenderContext::Backend* lastRenderBackend = nullptr;

	public:
		TestFrame(const spk::Rect2D& p_rect, std::string p_title) :
			currentRect(p_rect),
			currentTitle(std::move(p_title))
		{
		}

		void resize(const spk::Rect2D& p_rect) override
		{
			currentRect = p_rect;
			++resizeCount;
			resizeHistory.push_back(p_rect);
		}

		void setTitle(const std::string& p_title) override
		{
			currentTitle = p_title;
			++setTitleCount;
			titleHistory.push_back(p_title);
		}

		void requestClosure() override
		{
			++requestClosureCount;
		}

		std::unique_ptr<spk::IRenderContext> createRenderContext(spk::IRenderContext::Backend& p_backend) override
		{
			++createRenderContextCount;
			lastRenderBackend = &p_backend;
			return p_backend.createRenderContext(*this);
		}

		[[nodiscard]] spk::Rect2D rect() const override
		{
			return currentRect;
		}

		[[nodiscard]] std::string title() const override
		{
			return currentTitle;
		}

		void emitMouseEvent(const spk::Event& p_event)
		{
			_emitMouseEvent(p_event);
		}

		void emitKeyboardEvent(const spk::Event& p_event)
		{
			_emitKeyboardEvent(p_event);
		}

		void emitFrameEvent(const spk::Event& p_event)
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
			spk::Event event;
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
				return nullptr;
			}

			auto result = std::make_unique<TestFrame>(p_rect, p_title);
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
					_frame().emitMouseEvent(pendingEvent.event);
					break;
				case PendingEventKind::Keyboard:
					_frame().emitKeyboardEvent(pendingEvent.event);
					break;
				case PendingEventKind::Frame:
					_frame().emitFrameEvent(pendingEvent.event);
					break;
				}
			}

			if (onPollEvents != nullptr)
			{
				onPollEvents(*this);
			}
		}

		template <typename TPayloadType>
		void emitMousePayload(TPayloadType p_payload)
		{
			_frame().emitMouseEvent(spk::Event(std::move(p_payload)));
		}

		template <typename TPayloadType>
		void emitKeyboardPayload(TPayloadType p_payload)
		{
			_frame().emitKeyboardEvent(spk::Event(std::move(p_payload)));
		}

		template <typename TPayloadType>
		void emitFramePayload(TPayloadType p_payload)
		{
			_frame().emitFrameEvent(spk::Event(std::move(p_payload)));
		}

		template <typename TPayloadType>
		void queueMousePayload(TPayloadType p_payload)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Mouse,
				.event = spk::Event(std::move(p_payload))
			});
		}

		template <typename TPayloadType>
		void queueKeyboardPayload(TPayloadType p_payload)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Keyboard,
				.event = spk::Event(std::move(p_payload))
			});
		}

		template <typename TPayloadType>
		void queueFramePayload(TPayloadType p_payload)
		{
			_pendingEvents.push_back(PendingEvent{
				.kind = PendingEventKind::Frame,
				.event = spk::Event(std::move(p_payload))
			});
		}
	};

	class TestRenderContext : public spk::IRenderContext
	{
	public:
		int makeCurrentCount = 0;
		int presentCount = 0;
		int setVSyncCount = 0;
		int notifyResizeCount = 0;
		bool lastVSync = false;
		spk::Rect2D lastResizeRect;
		std::vector<spk::Rect2D> resizeHistory;

	public:
		void makeCurrent() override
		{
			++makeCurrentCount;
		}

		void present() override
		{
			++presentCount;
		}

		void setVSync(bool p_enabled) override
		{
			++setVSyncCount;
			lastVSync = p_enabled;
		}

		void notifyResize(const spk::Rect2D& p_rect) override
		{
			++notifyResizeCount;
			lastResizeRect = p_rect;
			resizeHistory.push_back(p_rect);
		}
	};

	class TestRenderContextBackend : public spk::IRenderContext::Backend
	{
	public:
		int createRenderContextCount = 0;
		bool returnNullContext = false;
		spk::IFrame* lastFrame = nullptr;
		TestRenderContext* createdContext = nullptr;

	public:
		std::unique_ptr<spk::IRenderContext> createRenderContext(spk::IFrame& p_frame) override
		{
			++createRenderContextCount;
			lastFrame = &p_frame;

			if (returnNullContext == true)
			{
				createdContext = nullptr;
				return nullptr;
			}

			auto result = std::make_unique<TestRenderContext>();
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
		void _appendRenderCommands(spk::RenderCommandBuilder& p_builder) const override
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

			p_builder.emplace<CallbackRenderCommand>(
				[this]()
				{
					++renderCount;
					callLog.push_back(name() + ":render");
					if (sharedCallLog != nullptr)
					{
						sharedCallLog->push_back(name() + ":render");
					}
				});
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

		void _onFrameEvent(const spk::Event& p_event) override
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
				p_event.metadata.isConsumed = true;
			}
		}

		void _onMouseEvent(const spk::Event& p_event) override
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
				p_event.metadata.isConsumed = true;
			}
		}

		void _onKeyboardEvent(const spk::Event& p_event) override
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
				p_event.metadata.isConsumed = true;
			}
		}
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

		void _appendRenderCommands(spk::RenderCommandBuilder& p_builder) const override
		{
			if (onAppendRenderCommands != nullptr)
			{
				onAppendRenderCommands();
			}

			if (onExecuteRenderCommand != nullptr)
			{
				p_builder.emplace<CallbackRenderCommand>(onExecuteRenderCommand);
			}
			else if (onRender != nullptr)
			{
				p_builder.emplace<CallbackRenderCommand>(onRender);
			}
		}
	};

	struct WindowHostBundle
	{
		TestPlatformRuntime* platformRuntime = nullptr;
		TestRenderContextBackend* renderBackend = nullptr;
		std::unique_ptr<spk::WindowHost> windowHost = nullptr;
	};

	inline WindowHostBundle createWindowHostBundle(const spk::Rect2D& p_rect = defaultRect(), const std::string& p_title = "TestWindow")
	{
		auto platformRuntime = std::make_shared<TestPlatformRuntime>();
		auto renderBackend = std::make_unique<TestRenderContextBackend>();

		WindowHostBundle result;
		result.platformRuntime = platformRuntime.get();
		result.renderBackend = renderBackend.get();
		result.windowHost = std::make_unique<spk::WindowHost>(spk::WindowHost::Configuration{
			.rect = p_rect,
			.title = p_title,
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

		return result;
	}

	struct WindowBundle
	{
		TestPlatformRuntime* platformRuntime = nullptr;
		TestRenderContextBackend* renderBackend = nullptr;
		std::unique_ptr<spk::Window> window = nullptr;
	};

	inline WindowBundle createWindowBundle(const spk::Rect2D& p_rect = defaultRect(), const std::string& p_title = "Window")
	{
		auto platformRuntime = std::make_shared<TestPlatformRuntime>();
		auto renderBackend = std::make_unique<TestRenderContextBackend>();

		WindowBundle result;
		result.platformRuntime = platformRuntime.get();
		result.renderBackend = renderBackend.get();
		result.window = std::make_unique<spk::Window>(spk::WindowHost::Configuration{
			.rect = p_rect,
			.title = p_title,
			.platformRuntime = std::move(platformRuntime),
			.renderBackend = std::move(renderBackend)
		});

		return result;
	}
}