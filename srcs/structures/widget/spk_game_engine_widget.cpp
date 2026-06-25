#include "structures/widget/spk_game_engine_widget.hpp"

namespace spk
{
	GameEngineWidget::GameEngineWidget(const std::string &p_name, spk::Widget *p_parent) :
		spk::Widget(p_name, p_parent)
	{
	}

	GameEngineWidget::~GameEngineWidget() = default;

	void GameEngineWidget::_onUpdate(const spk::UpdateTick &p_tick)
	{
		_gameEngine.update(p_tick);
		_gameEngine.synchronize();
	}

	spk::RenderUnit GameEngineWidget::_buildRenderUnit() const
	{
		spk::RenderUnitBuilder builder;

		_gameEngine.render(builder);

		return builder.build();
	}

	void GameEngineWidget::_onGeometryChange()
	{
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
