#pragma once

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_rect_2d.hpp"
#include "spk_timestamp.hpp"
#include "spk_vector2.hpp"

namespace spk
{
	struct EventRecord
	{
		spk::Timestamp timestamp;
		std::uint64_t sequence = 0;

		EventRecord() = default;

		explicit EventRecord(const spk::Timestamp& p_timestamp) :
			timestamp(p_timestamp)
		{
		}
	};

	template <typename TRecord>
	class EventView
	{
	private:
		static_assert(std::is_base_of_v<spk::EventRecord, TRecord>, "EventView<TRecord> requires an EventRecord-derived type");

		const TRecord* _record = nullptr;
		bool _isConsumed = false;

	public:
		explicit EventView(const TRecord& p_record) :
			_record(&p_record)
		{
		}

		template <typename TOtherRecord>
		EventView(const EventView<TOtherRecord>&) = delete;

		[[nodiscard]] bool isConsumed() const
		{
			return _isConsumed;
		}

		void consume()
		{
			_isConsumed = true;
		}

		[[nodiscard]] const spk::Timestamp& timestamp() const
		{
			return _record->timestamp;
		}

		[[nodiscard]] const TRecord& record() const
		{
			return *_record;
		}

		[[nodiscard]] const TRecord& data() const
		{
			return *_record;
		}

		[[nodiscard]] const TRecord* operator->() const
		{
			return _record;
		}

		[[nodiscard]] const TRecord& operator*() const
		{
			return *_record;
		}
	};

	template <typename TRecord, typename TDeviceType>
	class DeviceEventView : public EventView<TRecord>
	{
	private:
		const TDeviceType* _device = nullptr;

	public:
		DeviceEventView(const TRecord& p_record, const TDeviceType& p_device) :
			EventView<TRecord>(p_record),
			_device(&p_device)
		{
		}

		[[nodiscard]] const TDeviceType& device() const
		{
			return *_device;
		}
	};

	struct WindowCloseRequestedRecord : public EventRecord
	{
		using EventRecord::EventRecord;
		// Event corresponds to WM_CLOSE on WinAPI.
	};

	struct WindowDestroyedRecord : public EventRecord
	{
		using EventRecord::EventRecord;
		// Event corresponds to the point where the native window runtime is effectively destroyed.
		// Emitting this event invalidates the frame surface immediately, before subscribers are notified.
	};

	struct WindowMovedRecord : public EventRecord
	{
		spk::Vector2Int position;
	};

	struct WindowResizedRecord : public EventRecord
	{
		spk::Rect2D rect;
	};

	struct WindowFocusGainedRecord : public EventRecord
	{
		using EventRecord::EventRecord;
	};

	struct WindowFocusLostRecord : public EventRecord
	{
		using EventRecord::EventRecord;
	};

	struct WindowShownRecord : public EventRecord
	{
		using EventRecord::EventRecord;
	};

	struct WindowHiddenRecord : public EventRecord
	{
		using EventRecord::EventRecord;
	};

	struct MouseEnteredRecord : public EventRecord
	{
		spk::Vector2Int position;
	};

	struct MouseLeftRecord : public EventRecord
	{
		spk::Vector2Int position;
	};

	struct MouseMovedRecord : public EventRecord
	{
		spk::Vector2Int position;
		spk::Vector2Int delta;
	};

	struct MouseWheelScrolledRecord : public EventRecord
	{
		spk::Vector2 delta;
	};

	struct MouseButtonPressedRecord : public EventRecord
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct MouseButtonReleasedRecord : public EventRecord
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct MouseButtonDoubleClickedRecord : public EventRecord
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct KeyPressedRecord : public EventRecord
	{
		spk::Keyboard::Key key = spk::Keyboard::Unknow;
		bool isRepeated = false;
	};

	struct KeyReleasedRecord : public EventRecord
	{
		spk::Keyboard::Key key = spk::Keyboard::Unknow;
	};

	struct TextInputRecord : public EventRecord
	{
		char32_t glyph = U'\0';
	};

	using FrameEventRecord = std::variant<
		WindowCloseRequestedRecord,
		WindowDestroyedRecord,
		WindowMovedRecord,
		WindowResizedRecord,
		WindowFocusGainedRecord,
		WindowFocusLostRecord,
		WindowShownRecord,
		WindowHiddenRecord>;

	using MouseEventRecord = std::variant<
		MouseEnteredRecord,
		MouseLeftRecord,
		MouseMovedRecord,
		MouseWheelScrolledRecord,
		MouseButtonPressedRecord,
		MouseButtonReleasedRecord,
		MouseButtonDoubleClickedRecord>;

	using KeyboardEventRecord = std::variant<
		KeyPressedRecord,
		KeyReleasedRecord,
		TextInputRecord>;

	template <typename... TVisitors>
	struct Overloaded : TVisitors...
	{
		using TVisitors::operator()...;
	};

	template <typename... TVisitors>
	Overloaded(TVisitors...) -> Overloaded<TVisitors...>;

	template <typename TRecord>
	[[nodiscard]] std::decay_t<TRecord> makeEventRecord(TRecord&& p_record)
	{
		return std::forward<TRecord>(p_record);
	}

	template <typename TRecord, typename TVariant>
	[[nodiscard]] bool holds(const TVariant& p_event)
	{
		return std::holds_alternative<TRecord>(p_event);
	}

	template <typename TRecord, typename TVariant>
	[[nodiscard]] TRecord* getIf(TVariant& p_event)
	{
		return std::get_if<TRecord>(&p_event);
	}

	template <typename TRecord, typename TVariant>
	[[nodiscard]] const TRecord* getIf(const TVariant& p_event)
	{
		return std::get_if<TRecord>(&p_event);
	}

	template <typename TVariant>
	[[nodiscard]] std::uint64_t eventSequence(const TVariant& p_event)
	{
		return std::visit(
			[](const auto& p_record) -> std::uint64_t
			{
				return p_record.sequence;
			},
			p_event);
	}

	template <typename TVariant>
	void setEventSequence(TVariant& p_event, std::uint64_t p_sequence)
	{
		std::visit(
			[p_sequence](auto& p_record)
			{
				p_record.sequence = p_sequence;
			},
			p_event);
	}

	using WindowCloseRequestedEvent = spk::EventView<WindowCloseRequestedRecord>;
	using WindowDestroyedEvent = spk::EventView<WindowDestroyedRecord>;
	using WindowMovedEvent = spk::EventView<WindowMovedRecord>;
	using WindowResizedEvent = spk::EventView<WindowResizedRecord>;
	using WindowFocusGainedEvent = spk::EventView<WindowFocusGainedRecord>;
	using WindowFocusLostEvent = spk::EventView<WindowFocusLostRecord>;
	using WindowShownEvent = spk::EventView<WindowShownRecord>;
	using WindowHiddenEvent = spk::EventView<WindowHiddenRecord>;

	using MouseEnteredWindowEvent = spk::DeviceEventView<MouseEnteredRecord, spk::Mouse>;
	using MouseLeftWindowEvent = spk::DeviceEventView<MouseLeftRecord, spk::Mouse>;
	using MouseMovedEvent = spk::DeviceEventView<MouseMovedRecord, spk::Mouse>;
	using MouseWheelScrolledEvent = spk::DeviceEventView<MouseWheelScrolledRecord, spk::Mouse>;
	using MouseButtonPressedEvent = spk::DeviceEventView<MouseButtonPressedRecord, spk::Mouse>;
	using MouseButtonReleasedEvent = spk::DeviceEventView<MouseButtonReleasedRecord, spk::Mouse>;
	using MouseButtonDoubleClickedEvent = spk::DeviceEventView<MouseButtonDoubleClickedRecord, spk::Mouse>;

	using KeyPressedEvent = spk::DeviceEventView<KeyPressedRecord, spk::Keyboard>;
	using KeyReleasedEvent = spk::DeviceEventView<KeyReleasedRecord, spk::Keyboard>;
	using TextInputEvent = spk::DeviceEventView<TextInputRecord, spk::Keyboard>;
}
