#pragma once

#include <optional>

#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_render_command_builder.hpp"
#include "spk_timestamp.hpp"
#include "spk_widget.hpp"

namespace spk
{
	class IModule
	{
	private:
		spk::Widget* _widget = nullptr;

	public:
		virtual ~IModule();

		virtual void bind(spk::Widget* p_widget);

		[[nodiscard]] spk::Widget* widget();
		[[nodiscard]] const spk::Widget* widget() const;
	};

	class MouseModule : public IModule
	{
	private:
		spk::Mouse _mouse;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		MouseModule();

		void pushEvent(const spk::Event& p_event);

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;
	};

	class KeyboardModule : public IModule
	{
	private:
		spk::Keyboard _keyboard;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		KeyboardModule();

		void pushEvent(const spk::Event& p_event);

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;
	};

	class FrameModule : public IModule
	{
	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		FrameModule();

		void pushEvent(const spk::Event& p_event);
	};

	class UpdateModule : public IModule
	{
	private:
		std::optional<spk::Timestamp> _lastTimestamp;
		spk::Mouse* _mouse = nullptr;
		spk::Keyboard* _keyboard = nullptr;

	public:
		UpdateModule();

		void bindInputs(spk::Mouse* p_mouse, spk::Keyboard* p_keyboard);
		void update();
	};

	class RenderModule
	{
	public:
		RenderModule();

		void render(const spk::RenderCommandBuilder& p_builder) const;
	};
}