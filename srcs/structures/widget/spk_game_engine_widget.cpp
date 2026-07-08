#include "structures/widget/spk_game_engine_widget.hpp"

#include <array>

#include <GL/glew.h>

#include "structures/graphics/opengl/spk_opengl_clear_command.hpp"
#include "structures/graphics/rendering/command/spk_image_render_command.hpp"
#include "structures/graphics/rendering/command/spk_use_framebuffer_render_command.hpp"

namespace spk
{
	GameEngineWidget::GameEngineWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
	}

	GameEngineWidget::~GameEngineWidget() = default;

	void GameEngineWidget::_onUpdate(const spk::UpdateContext &p_tick)
	{
		_gameEngine.update(p_tick);
	}

	spk::RenderUnit GameEngineWidget::_buildRenderUnit() const
	{
		const spk::Vector2UInt &frameBufferSize = _frameBuffer.size();
		if (frameBufferSize.x == 0 || frameBufferSize.y == 0)
		{
			return spk::RenderUnit();
		}

		spk::RenderUnitBuilder builder;

		// 1. Bind the off-screen framebuffer, clear it (color + depth, so entities
		//    depth-sort against a fresh buffer each frame) and render the engine
		//    into it through the framebuffer's own viewport.
		builder.emplace<spk::UseFrameBufferRenderCommand>(&_frameBuffer, _frameBuffer.viewport());
		builder.emplace<spk::ClearCommand>(
			std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f},
			static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
		builder.add(_gameEngine.buildRenderUnit());

		// 2. Return to the screen and blit the color attachment onto this widget's
		//    rectangle. The section is vertically flipped because a render-to-
		//    texture target is stored bottom-up relative to screen coordinates.
		builder.emplace<spk::UseFrameBufferRenderCommand>(nullptr, viewport());

		const spk::Texture::Section flippedSection({0.0f, 1.0f}, {1.0f, -1.0f});
		const spk::Rect2D localRect({0, 0}, frameBufferSize);
		builder.emplace<spk::ImageRenderCommand>(_frameBuffer.colorAttachment(), flippedSection, localRect);

		return builder.build();
	}

	void GameEngineWidget::_onGeometryChange()
	{
		_frameBuffer.resize(geometry().size);
		invalidateRenderUnit();
	}

	void GameEngineWidget::_onWindowCloseRequestedEvent(spk::WindowCloseRequestedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowDestroyedEvent(spk::WindowDestroyedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowMovedEvent(spk::WindowMovedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowResizedEvent(spk::WindowResizedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowFocusGainedEvent(spk::WindowFocusGainedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowFocusLostEvent(spk::WindowFocusLostEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowShownEvent(spk::WindowShownEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onWindowHiddenEvent(spk::WindowHiddenEvent &p_event) { _forward(p_event); }

	void GameEngineWidget::_onMouseEnteredEvent(spk::MouseEnteredWindowEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseLeftEvent(spk::MouseLeftWindowEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseMovedEvent(spk::MouseMovedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseWheelScrolledEvent(spk::MouseWheelScrolledEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseButtonReleasedEvent(spk::MouseButtonReleasedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onMouseButtonDoubleClickedEvent(spk::MouseButtonDoubleClickedEvent &p_event) { _forward(p_event); }

	void GameEngineWidget::_onKeyPressedEvent(spk::KeyPressedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onKeyReleasedEvent(spk::KeyReleasedEvent &p_event) { _forward(p_event); }
	void GameEngineWidget::_onTextInputEvent(spk::TextInputEvent &p_event) { _forward(p_event); }

	spk::GameEngine &GameEngineWidget::gameEngine()
	{
		return _gameEngine;
	}

	const spk::GameEngine &GameEngineWidget::gameEngine() const
	{
		return _gameEngine;
	}
}
