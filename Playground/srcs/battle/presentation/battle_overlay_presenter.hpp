#pragma once

#include "battle/presentation/battle_overlay_model.hpp"

#include <cstdint>
#include <memory>

namespace spk
{
	class Entity3D;
	class GameEngine;
	class Texture;
	class TextureMeshRenderer3D;
}

namespace pg
{
	class IBattlePresentationCellSource;
	class BoardData;
	struct GameRules;

	class BattleOverlayPresenter
	{
	private:
		spk::GameEngine &_engine;
		const spk::Texture &_texture;
		const GameRules &_rules;
		std::unique_ptr<spk::Entity3D> _entity;
		spk::TextureMeshRenderer3D *_renderer = nullptr;
		std::uint64_t _presentedRevision = 0;
		bool _attached = false;

	public:
		BattleOverlayPresenter(spk::GameEngine &p_engine, const spk::Texture &p_texture, const GameRules &p_rules);
		~BattleOverlayPresenter();
		BattleOverlayPresenter(const BattleOverlayPresenter &) = delete;
		BattleOverlayPresenter &operator=(const BattleOverlayPresenter &) = delete;

		void attach();
		void present(const BattleOverlayModel &p_model, const BoardData &p_board, const IBattlePresentationCellSource &p_cells);
		void detach() noexcept;
	};
}
