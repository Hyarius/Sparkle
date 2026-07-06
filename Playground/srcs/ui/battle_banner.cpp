#include "ui/battle_banner.hpp"

#include "structures/graphics/texture/spk_font.hpp"

#include <algorithm>

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
		if (_notification && isActivated())
		{
			_queuedResult = p_winner;
			_queuedConfirmation = std::move(p_confirmation);
			return;
		}
		_notification = false;
		_confirmation = std::move(p_confirmation);
		_elapsedSeconds = 0.0f;
		_autoHideSeconds = AutoConfirmSeconds;
		_label.setText(
			p_winner == BattleSide::Player
				? "Victory!\nclick to continue"
				: "Defeat\xE2\x80\xA6\nclick to continue");
		activate();
	}

	void BattleBanner::showMessage(std::string p_message, float p_seconds)
	{
		_notification = true;
		_confirmation = nullptr;
		_elapsedSeconds = 0.0f;
		_autoHideSeconds = std::max(0.0f, p_seconds);
		_label.setText(std::move(p_message));
		activate();
	}

	void BattleBanner::hideResult()
	{
		deactivate();
		_confirmation = nullptr;
		_queuedResult.reset();
		_queuedConfirmation = nullptr;
		_notification = false;
		_elapsedSeconds = 0.0f;
		_autoHideSeconds = AutoConfirmSeconds;
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
		const bool wasNotification = _notification;
		_notification = false;
		deactivate();
		if (wasNotification && _queuedResult.has_value())
		{
			const BattleSide winner = *_queuedResult;
			_queuedResult.reset();
			auto queuedConfirmation = std::move(_queuedConfirmation);
			showResult(winner, std::move(queuedConfirmation));
			return;
		}
		if (confirmation)
		{
			confirmation();
		}
	}

	void BattleBanner::_onUpdate(const spk::UpdateTick &p_tick)
	{
		_elapsedSeconds += static_cast<float>(p_tick.deltaTime.seconds());
		if (_elapsedSeconds >= _autoHideSeconds)
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
