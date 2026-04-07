#pragma once

#include <type_traits>
#include <utility>
#include <variant>

#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_rect_2d.hpp"
#include "spk_timestamp.hpp"
#include "spk_duration.hpp"
#include "spk_vector2.hpp"

namespace spk
{
	struct WindowCloseRequestedPayload
	{
	};

	struct WindowMovedPayload
	{
		spk::Vector2Int position;
	};

	struct WindowResizedPayload
	{
		spk::Rect2D rect;
	};

	struct WindowFocusGainedPayload
	{
	};

	struct WindowFocusLostPayload
	{
	};

	struct WindowShownPayload
	{
	};

	struct WindowHiddenPayload
	{
	};

	struct UpdatePayload
	{
		spk::Duration deltaTime;

		spk::Mouse* mouse;
		spk::Keyboard* keyboard;
	};

	struct MouseEnteredPayload
	{
		spk::Vector2Int position;
	};

	struct MouseLeftPayload
	{
		spk::Vector2Int position;
	};

	struct MouseMovedPayload
	{
		spk::Vector2Int position;
		spk::Vector2Int delta;
	};

	struct MouseWheelScrolledPayload
	{
		spk::Vector2 delta;
	};

	struct MouseButtonPressedPayload
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct MouseButtonReleasedPayload
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct MouseButtonDoubleClickedPayload
	{
		spk::Mouse::Button button = spk::Mouse::Left;
	};

	struct KeyPressedPayload
	{
		spk::Keyboard::Key key = spk::Keyboard::Unknow;
		bool isRepeated = false;
	};

	struct KeyReleasedPayload
	{
		spk::Keyboard::Key key = spk::Keyboard::Unknow;
	};

	struct TextInputPayload
	{
		char32_t glyph = U'\0';
	};

	using EventPayload = std::variant<
		WindowCloseRequestedPayload,
		WindowMovedPayload,
		WindowResizedPayload,
		WindowFocusGainedPayload,
		WindowFocusLostPayload,
		WindowShownPayload,
		WindowHiddenPayload,
		UpdatePayload,
		MouseEnteredPayload,
		MouseLeftPayload,
		MouseMovedPayload,
		MouseWheelScrolledPayload,
		MouseButtonPressedPayload,
		MouseButtonReleasedPayload,
		MouseButtonDoubleClickedPayload,
		KeyPressedPayload,
		KeyReleasedPayload,
		TextInputPayload>;

	struct EventMetadata
	{
		bool isConsumed = false;
		spk::Timestamp timestamp;

		EventMetadata() = default;
		explicit EventMetadata(const spk::Timestamp& p_timestamp) :
			isConsumed(false),
			timestamp(p_timestamp)
		{
		}
	};

	struct Event
	{
	public:
		template <typename TPayloadType>
		struct View
		{
			static_assert(std::is_same_v<TPayloadType, std::decay_t<TPayloadType>>, "Event::View payload type must be non-cvref");

			EventMetadata& metadata;
			TPayloadType& payload;

			[[nodiscard]] bool isConsumed() const
			{
				return metadata.isConsumed;
			}

			void consume()
			{
				metadata.isConsumed = true;
			}

			[[nodiscard]] const spk::Timestamp& timestamp() const
			{
				return metadata.timestamp;
			}
		};

		template <typename TPayloadType, typename TDeviceType>
		struct DeviceView : public spk::Event::View<TPayloadType>
		{
			const TDeviceType& device;
		};

	public:
		EventMetadata metadata;
		EventPayload payload;

	public:
		Event() = default;

		template <typename TPayloadType>
		Event(TPayloadType&& p_payload) :
			metadata(),
			payload(std::forward<TPayloadType>(p_payload))
		{
		}

		template <typename TPayloadType>
		Event(const spk::Timestamp& p_timestamp, TPayloadType&& p_payload) :
			metadata(p_timestamp),
			payload(std::forward<TPayloadType>(p_payload))
		{
		}

		template <typename TPayloadType>
		Event(const EventMetadata& p_metadata, TPayloadType&& p_payload) :
			metadata(p_metadata),
			payload(std::forward<TPayloadType>(p_payload))
		{
		}

		template <typename TPayloadType>
		[[nodiscard]] bool holds() const
		{
			return std::holds_alternative<TPayloadType>(payload);
		}

		template <typename TPayloadType>
		[[nodiscard]] TPayloadType* getIf()
		{
			return std::get_if<TPayloadType>(&payload);
		}

		template <typename TPayloadType>
		[[nodiscard]] const TPayloadType* getIf() const
		{
			return std::get_if<TPayloadType>(&payload);
		}

		template <typename TPayloadType>
		[[nodiscard]] View<TPayloadType> view()
		{
			return View<TPayloadType>{metadata, std::get<TPayloadType>(payload)};
		}

		template <typename TPayloadType, typename TDeviceType>
		[[nodiscard]] DeviceView<TPayloadType, TDeviceType> deviceView(const TDeviceType& p_device)
		{
			return DeviceView<TPayloadType, TDeviceType>{
				{ metadata, std::get<TPayloadType>(payload) },
				p_device
			};
		}
	};

	using WindowCloseRequestedEvent = spk::Event::View<WindowCloseRequestedPayload>;
	using WindowMovedEvent = spk::Event::View<WindowMovedPayload>;
	using WindowResizedEvent = spk::Event::View<WindowResizedPayload>;
	using WindowFocusGainedEvent = spk::Event::View<WindowFocusGainedPayload>;
	using WindowFocusLostEvent = spk::Event::View<WindowFocusLostPayload>;
	using WindowShownEvent = spk::Event::View<WindowShownPayload>;
	using WindowHiddenEvent = spk::Event::View<WindowHiddenPayload>;

	using UpdateEvent = spk::Event::View<UpdatePayload>;

	using MouseEnteredEvent = spk::Event::DeviceView<MouseEnteredPayload, spk::Mouse>;
	using MouseLeftEvent = spk::Event::DeviceView<MouseLeftPayload, spk::Mouse>;
	using MouseMovedEvent = spk::Event::DeviceView<MouseMovedPayload, spk::Mouse>;
	using MouseWheelScrolledEvent = spk::Event::DeviceView<MouseWheelScrolledPayload, spk::Mouse>;
	using MouseButtonPressedEvent = spk::Event::DeviceView<MouseButtonPressedPayload, spk::Mouse>;
	using MouseButtonReleasedEvent = spk::Event::DeviceView<MouseButtonReleasedPayload, spk::Mouse>;
	using MouseButtonDoubleClickedEvent = spk::Event::DeviceView<MouseButtonDoubleClickedPayload, spk::Mouse>;

	using KeyPressedEvent = spk::Event::DeviceView<KeyPressedPayload, spk::Keyboard>;
	using KeyReleasedEvent = spk::Event::DeviceView<KeyReleasedPayload, spk::Keyboard>;
	using TextInputEvent = spk::Event::DeviceView<TextInputPayload, spk::Keyboard>;
}