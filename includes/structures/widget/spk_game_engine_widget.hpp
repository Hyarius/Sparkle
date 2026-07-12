#pragma once

#include <string>

#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/graphics/spk_framebuffer_object.hpp"
#include "structures/widget/spk_widget.hpp"

namespace spk
{
	class GameEngineWidget : public spk::Widget
	{
	private:
		mutable spk::GameEngine _gameEngine;
		spk::FrameBufferObject _frameBuffer;

		template <typename TEvent>
		void _forward(TEvent &p_event)
		{
			_gameEngine.dispatchEvent(p_event);
		}

	protected:
		void _onUpdate(const spk::UpdateContext &p_tick) override;
		[[nodiscard]] spk::RenderUnit _buildRenderUnit() const override;
		void _onGeometryChange() override;

		void _onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event) override;
		void _onWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event) override;
		void _onWindowMovedEvent(spk::WindowMovedEvent &p_event) override;
		void _onWindowResizedEvent(spk::WindowResizedEvent &p_event) override;
		void _onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event) override;
		void _onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event) override;
		void _onWindowShownEvent(spk::WindowShownEvent &p_event) override;
		void _onWindowHiddenEvent(spk::WindowHiddenEvent &p_event) override;

		void _onMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event) override;
		void _onMouseLeftEvent(spk::MouseLeftWindowEvent &p_event) override;
		void _onMouseMovedEvent(spk::MouseMovedEvent &p_event) override;
		void _onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) override;
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) override;
		void _onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) override;
		void _onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event) override;

		void _onKeyPressedEvent(spk::KeyPressedEvent &p_event) override;
		void _onKeyReleasedEvent(spk::KeyReleasedEvent &p_event) override;
		void _onTextInputEvent(spk::TextInputEvent &p_event) override;

	public:
		explicit GameEngineWidget(const std::string &p_name, spk::Widget *p_parent = nullptr);
		~GameEngineWidget() override;

		[[nodiscard]] spk::GameEngine &gameEngine();
		[[nodiscard]] const spk::GameEngine &gameEngine() const;
		[[nodiscard]] const spk::FrameBufferObject &frameBuffer() const noexcept;
	};
}
