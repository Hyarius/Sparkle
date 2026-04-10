#pragma once

#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_widget.hpp"
#include "spk_window_host.hpp"

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
		spk::WindowHost* _windowHost = nullptr;
		spk::IFrame::EventContract _mouseEventContract;
		spk::Mouse _mouse;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		MouseModule();

		void bindWindowHost(spk::WindowHost* p_windowHost);

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;
	};

	class KeyboardModule : public IModule
	{
	private:
		spk::WindowHost* _windowHost = nullptr;
		spk::IFrame::EventContract _keyboardEventContract;
		spk::Keyboard _keyboard;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		KeyboardModule();

		void bindWindowHost(spk::WindowHost* p_windowHost);

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;
	};

	class FrameModule : public IModule
	{
	private:
		spk::IFrame::EventContract _frameEventContract;
		spk::WindowHost* _windowHost = nullptr;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		FrameModule();

		void bindWindowHost(spk::WindowHost* p_windowHost);
	};

	class UpdateModule : public IModule
	{
	public:
		UpdateModule();

		void update(const spk::UpdateTick& p_tick);
	};

	class RenderModule : public IModule
	{
	public:
		RenderModule();

		void render();
	};
}
