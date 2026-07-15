#include "battle/presentation/battle_overlay_presenter.hpp"

#include "battle/presentation/battle_presentation_cell_source.hpp"
#include "core/game_rules.hpp"
#include "rendering/walk_surface_mask_mesh_builder.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"

#include <stdexcept>
#include <vector>

namespace pg
{
	BattleOverlayPresenter::BattleOverlayPresenter(
		spk::GameEngine &p_engine,
		const spk::Texture &p_texture,
		const GameRules &p_rules) :
		_engine(p_engine),
		_texture(p_texture),
		_rules(p_rules)
	{
	}

	BattleOverlayPresenter::~BattleOverlayPresenter()
	{
		detach();
	}

	void BattleOverlayPresenter::attach()
	{
		if (_attached)
		{
			return;
		}
		_entity = std::make_unique<spk::Entity3D>();
		_renderer = &_entity->addComponent<spk::TextureMeshRenderer3D>();
		_renderer->setTexture(&_texture);
		_renderer->setTint({1.0f, 1.0f, 1.0f, 0.68f});
		_renderer->setTranslucent(true);
		_renderer->setCastsShadows(false);
		_renderer->setReceivesShadows(false);
		_renderer->setMesh(std::make_shared<spk::TextureMesh3D>());
		_engine.addEntity(_entity.get());
		_attached = true;
		_presentedRevision = 0;
	}

	void BattleOverlayPresenter::present(
		const BattleOverlayModel &p_model,
		const BoardData &p_board,
		const IBattlePresentationCellSource &p_cells)
	{
		if (!_attached)
		{
			throw std::logic_error("battle overlay presenter is not attached");
		}
		if (_presentedRevision == p_model.revision)
		{
			return;
		}
		std::vector<WalkSurfaceMaskInstance> instances;
		instances.reserve(p_model.cells.size());
		for (const BattleMaskCell &cell : p_model.cells)
		{
			const auto iterator = _rules.overlayMasks.find(std::string(overlayMaskKey(cell.kind)));
			if (iterator == _rules.overlayMasks.end())
			{
				throw std::logic_error("configured battle overlay mask is absent from game rules");
			}
			const spk::Vector3Int support = p_board.toPresentationCell(cell.cell);
			if (p_cells.tryCell(support) != nullptr)
			{
				instances.push_back({.supportCell = support, .atlasCell = {iterator->second[0], iterator->second[1]}});
			}
		}
		_renderer->setMesh(std::make_shared<spk::TextureMesh3D>(WalkSurfaceMaskMeshBuilder::build(p_cells, instances)));
		_presentedRevision = p_model.revision;
	}

	void BattleOverlayPresenter::detach() noexcept
	{
		if (!_attached)
		{
			return;
		}
		_engine.removeEntity(_entity.get());
		_renderer = nullptr;
		_entity.reset();
		_attached = false;
		_presentedRevision = 0;
	}
}
