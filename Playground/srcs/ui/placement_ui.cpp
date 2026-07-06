#include "ui/placement_ui.hpp"

#include "battle/battle_unit.hpp"

#include <utility>

namespace pg
{
	PlacementUI::PlacementUI(const std::string &p_name, spk::Widget *p_parent) :
		spk::Panel(p_name, p_parent),
		_title(p_name + "/Title", this),
		_instructions(p_name + "/Instructions", this),
		_autoPlace(p_name + "/Auto", "Auto", this),
		_confirm(p_name + "/Confirm", "Confirm", this),
		_autoContract(_autoPlace.subscribeToClick([this] {
			if (_phase != nullptr) (void)_phase->autoPlacePlayer();
		})),
		_confirmContract(_confirm.subscribeToClick([this] {
			if (_phase != nullptr) (void)_phase->confirm();
		}))
	{
		_title.setText("Team placement");
		_title.setTextSize(spk::Font::Size(20, 1));
		_instructions.setTextSize(spk::Font::Size(14, 1));
		deactivate();
	}

	void PlacementUI::bind(PlacementPhase &p_phase)
	{
		unbind();
		_phase = &p_phase;
		_changeContract = _phase->subscribeToChange([this] { _refresh(); });
		_refresh();
	}

	void PlacementUI::unbind()
	{
		_changeContract.resign();
		_phase = nullptr;
		deactivate();
		if (_selectionChanged) _selectionChanged(nullptr);
	}

	bool PlacementUI::selectUnit(BattleUnit *p_unit)
	{
		if (_phase == nullptr || !_phase->select(p_unit))
		{
			return false;
		}
		_refresh();
		return true;
	}

	void PlacementUI::setSelectionChangedHandler(std::function<void(BattleUnit *)> p_handler)
	{
		_selectionChanged = std::move(p_handler);
	}

	void PlacementUI::_refresh()
	{
		if (_phase == nullptr || !_phase->active() || !_phase->interactivePlayer())
		{
			deactivate();
			if (_selectionChanged) _selectionChanged(nullptr);
			return;
		}
		activate();
		BattleUnit *unit = _phase->selected();
		_instructions.setText(unit == nullptr
			? "Select a card, then click a blue deployment cell."
			: "Placing " + unit->displayName() + ". Click a deployment cell.");
		const spk::Color confirmColor = _phase->canConfirm()
			? spk::Color{0.45f, 1.0f, 0.45f, 1.0f}
			: spk::Color{0.45f, 0.45f, 0.45f, 1.0f};
		_confirm.releasedLabel().setGlyphColor(confirmColor);
		_confirm.pressedLabel().setGlyphColor(confirmColor);
		if (_selectionChanged) _selectionChanged(unit);
	}

	void PlacementUI::_onGeometryChange()
	{
		spk::Panel::_onGeometryChange();
		const unsigned int width = geometry().width();
		_title.setGeometry(spk::Rect2D(8, 4, width > 16 ? width - 16 : 0, 28));
		_instructions.setGeometry(spk::Rect2D(8, 34, width > 216 ? width - 216 : 0, 38));
		_autoPlace.setGeometry(spk::Rect2D(
			static_cast<int>(width > 200 ? width - 200 : 0), 32, 90, 40));
		_confirm.setGeometry(spk::Rect2D(
			static_cast<int>(width > 104 ? width - 104 : 0), 32, 96, 40));
	}

	bool PlacementUI::bound() const noexcept { return _phase != nullptr; }
	bool PlacementUI::canConfirm() const noexcept { return _phase != nullptr && _phase->canConfirm(); }
	BattleUnit *PlacementUI::selected() const noexcept { return _phase != nullptr ? _phase->selected() : nullptr; }
	const spk::Font::Text &PlacementUI::instructionText() const { return _instructions.text(); }
}
