#pragma once

#include "spk_keyboard.hpp"
#include "spk_mouse.hpp"
#include "spk_widget.hpp"
#include "spk_window.hpp"

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
		spk::Window* _window = nullptr;
		spk::IFrame::Backend::EventContract _mouseEventContract;
		spk::Mouse _mouse;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		MouseModule();

		void bindWindow(spk::Window* p_window);

		[[nodiscard]] spk::Mouse& mouse();
		[[nodiscard]] const spk::Mouse& mouse() const;
	};

	class KeyboardModule : public IModule
	{
	private:
		spk::Window* _window = nullptr;
		spk::IFrame::Backend::EventContract _keyboardEventContract;
		spk::Keyboard _keyboard;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		KeyboardModule();

		void bindWindow(spk::Window* p_window);

		[[nodiscard]] spk::Keyboard& keyboard();
		[[nodiscard]] const spk::Keyboard& keyboard() const;
	};

	class FrameModule : public IModule
	{
	private:
		spk::IFrame::Backend::EventContract _frameEventContract;
		spk::Window* _window = nullptr;

	private:
		void _treatEvent(const spk::Event& p_event);

	public:
		FrameModule();

		void bindWindow(spk::Window* p_window);
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
