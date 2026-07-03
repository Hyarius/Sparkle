#include "ui/battle_banner.hpp"

#include "structures/graphics/texture/spk_font.hpp"

namespace pg
{
	BattleBanner::BattleBanner(const std::string &p_name, spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_label(p_name + "/Text", this)
	{
		_label.setTextSize(spk::Font::Size(30, 2));
		_label.setGlyphColor({1.0f, 1.0f, 1.0f, 1.0f});
		_label.setOutlineColor({0.0f, 0.0f, 0.0f, 1.0f});
		deactivate();
	}

	void BattleBanner::_onGeometryChange()
	{
		_label.setGeometry(spk::Rect2D({0, 0}, geometry().size));
	}

	void BattleBanner::showResult(BattleSide p_winner, std::function<void()> p_confirmation)
	{
		_confirmation = std::move(p_confirmation);
		_elapsedSeconds = 0.0f;
		_label.setText(
			p_winner == BattleSide::Player
				? "Victory!\nclick to continue"
				: "Defeat\xE2\x80\xA6\nclick to continue");
		activate();
	}

	void BattleBanner::hideResult()
	{
		deactivate();
		_confirmation = nullptr;
		_elapsedSeconds = 0.0f;
	}

	bool BattleBanner::showing() const noexcept
	{
		return isActivated();
	}

	void BattleBanner::_confirm()
	{
		if (!isActivated())
		{
			return;
		}
		auto confirmation = std::move(_confirmation);
		deactivate();
		if (confirmation)
		{
			confirmation();
		}
	}

	void BattleBanner::_onUpdate(const spk::UpdateTick &p_tick)
	{
		_elapsedSeconds += static_cast<float>(p_tick.deltaTime.seconds());
		if (_elapsedSeconds >= AutoConfirmSeconds)
		{
			_confirm();
		}
	}

	void BattleBanner::_onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &p_event)
	{
		if (p_event->button != spk::Mouse::Left || !absoluteGeometry().contains(p_event.device().position))
		{
			return;
		}
		p_event.consume();
		_confirm();
	}
}
