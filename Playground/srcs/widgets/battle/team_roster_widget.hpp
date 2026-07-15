#pragma once

#include "widgets/battle/team_roster_card.hpp"

#include "structures/widget/spk_panel.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace pg
{
	// Reusable vertical six-card roster.  It deliberately knows only the copied card view model
	// and a card interface, so a future frontend can supply a different card type without
	// duplicating roster selection, expansion or reflow behaviour.
	template <typename TCard, std::size_t Capacity = 6>
		requires std::derived_from<TCard, TeamRosterCard>
	class TeamRosterWidget final : public spk::Widget
	{
	public:
		using CardSelectionCallback = std::function<void(std::size_t)>;
		using LayoutChangedCallback = std::function<void()>;

	private:
		static constexpr std::size_t Padding = 6;
		static constexpr std::size_t Gap = 4;

		spk::Panel _background;
		std::array<std::unique_ptr<TCard>, Capacity> _cards;
		std::array<CreatureCardViewModel, Capacity> _models{};
		bool _actionable = false;
		std::optional<std::size_t> _selected;
		CardSelectionCallback _onCardSelected;
		CardSelectionCallback _onCardDetails;
		LayoutChangedCallback _onPreferredHeightChanged;

		void _renderCards()
		{
			for (std::size_t index = 0; index < Capacity; ++index)
			{
				_cards[index]->renderCard(_models[index], _selected == index, _actionable);
			}
		}

		void _select(std::size_t p_index, bool p_openDetails)
		{
			if (p_index >= Capacity || !_models[p_index].unit.has_value())
			{
				return;
			}
			_selected = p_index;
			_renderCards();
			// A selected card may have a different preferred height. Releasing the old
			// minimum and refreshing geometry gives the roster's parent an immediate,
			// deterministic reflow instead of waiting for a window resize.
			releaseMinimalSize();
			refreshGeometry();
			if (_onPreferredHeightChanged)
			{
				_onPreferredHeightChanged();
			}
			if (p_openDetails)
			{
				if (_onCardDetails)
				{
					_onCardDetails(p_index);
				}
			}
			else if (_onCardSelected)
			{
				_onCardSelected(p_index);
			}
		}

		void _layout()
		{
			_background.setGeometry(geometry().atOrigin());
			const std::size_t width = geometry().width();
			const std::size_t cardWidth = width > Padding * 2 ? width - Padding * 2 : width;
			std::size_t y = Padding;
			for (const std::unique_ptr<TCard> &card : _cards)
			{
				const std::size_t height = card->rosterHeight();
				card->setGeometry({static_cast<int>(Padding), static_cast<int>(y), cardWidth, height});
				y += height + Gap;
			}
		}

	protected:
		void _onGeometryChange() override
		{
			_layout();
		}

	public:
		explicit TeamRosterWidget(const std::string &p_name, spk::Widget *p_parent = nullptr) :
			spk::Widget(p_name, p_parent),
			_background(p_name + "/background", this)
		{
			for (std::size_t index = 0; index < Capacity; ++index)
			{
				_cards[index] = std::make_unique<TCard>(p_name + "/card/" + std::to_string(index), this);
				_cards[index]->setOnSelectedClick([this, index] {
					_select(index, false);
				});
				_cards[index]->setOnDetailsClick([this, index] {
					_select(index, true);
				});
			}
			configureMinimalSizeGenerator([this]() {
				return spk::Vector2UInt{0, static_cast<unsigned int>(preferredHeight())};
			});
			_renderCards();
			activate();
		}

		void render(std::span<const CreatureCardViewModel> p_models, bool p_actionable)
		{
			_models = {};
			for (std::size_t index = 0; index < std::min(Capacity, p_models.size()); ++index)
			{
				_models[index] = p_models[index];
			}
			_actionable = p_actionable;
			if (_selected.has_value() && !_models[*_selected].unit.has_value())
			{
				_selected.reset();
			}
			_renderCards();
			releaseMinimalSize();
			refreshGeometry();
		}

		void setOnCardSelected(CardSelectionCallback p_callback)
		{
			_onCardSelected = std::move(p_callback);
		}

		void setOnCardDetails(CardSelectionCallback p_callback)
		{
			_onCardDetails = std::move(p_callback);
		}

		void setOnPreferredHeightChanged(LayoutChangedCallback p_callback)
		{
			_onPreferredHeightChanged = std::move(p_callback);
		}

		[[nodiscard]] std::size_t preferredHeight() const noexcept
		{
			std::size_t result = Padding * 2;
			for (std::size_t index = 0; index < Capacity; ++index)
			{
				result += _cards[index]->rosterHeight();
				if (index + 1 < Capacity)
				{
					result += Gap;
				}
			}
			return result;
		}

		[[nodiscard]] const CreatureCardViewModel &model(std::size_t p_index) const
		{
			return _models.at(p_index);
		}
	};
}
