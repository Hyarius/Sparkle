#include "battle/presentation/battle_unit_presenter.hpp"

#include "battle/presentation/battle_presentation_cell_source.hpp"
#include "battle/presentation/battle_unit_position.hpp"
#include "core/registries.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/system/event/spk_update_context.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace
{
	[[nodiscard]] bool shown(const pg::BattleUnitSnapshot &p_unit) noexcept
	{
		return p_unit.placed && p_unit.cell.has_value();
	}
}

namespace pg
{
	PlaceholderVisual BattleUnitPresenter::_visualFor(const BattleUnitSnapshot &p_unit) const
	{
		const CreatureSpeciesDefinition *species = _registries->species().tryGet(p_unit.speciesId);
		if (species != nullptr)
		{
			if (const CreatureFormDefinition *form = species->tryForm(p_unit.formId); form != nullptr)
			{
				return form->presentation;
			}
		}
		return fallbackPlaceholderVisual(p_unit.speciesId + "/" + p_unit.formId);
	}

	void BattleUnitPresenter::_createOrUpdate(const BattleUnitSnapshot &p_unit, bool p_snap)
	{
		if (!shown(p_unit))
		{
			_remove(p_unit.id);
			return;
		}
		const PlaceholderVisual visual = _visualFor(p_unit);
		const float scale = toWorldScale(visual);
		const float renderedHeight = BattleUnitBaseEdge * scale;
		auto [iterator, inserted] = _views.try_emplace(p_unit.id);
		BattleUnitView &view = iterator->second;
		if (inserted)
		{
			view.id = p_unit.id;
			view.entity = std::make_unique<spk::Entity3D>();
			view.transform = &view.entity->transform();
			view.renderer = &view.entity->addComponent<spk::TextureMeshRenderer3D>();
			view.renderer->setMesh(_cubeMesh);
			view.renderer->setTexture(_texture);
			view.renderer->setCastsShadows(true);
			view.renderer->setReceivesShadows(true);
			_engine->addEntity(view.entity.get());
		}
		view.renderer->setTint(toSpkColor(visual));
		view.transform->setScale({scale, scale, scale});
		view.renderedHeight = renderedHeight;
		view.authoritativeCell = *p_unit.cell;
		if (p_snap || !view.cosmetic.has_value())
		{
			view.transform->setPosition(battleUnitCenterPosition(_board->board, *p_unit.cell, renderedHeight));
		}
	}

	void BattleUnitPresenter::_remove(BattleUnitId p_id) noexcept
	{
		const auto iterator = _views.find(p_id);
		if (iterator == _views.end())
		{
			return;
		}
		BattleUnitView &view = iterator->second;
		view.cosmetic.reset();
		while (!view.queuedCosmetics.empty())
		{
			view.queuedCosmetics.pop();
		}
		if (_engine != nullptr && view.entity != nullptr)
		{
			_engine->removeEntity(view.entity.get());
		}
		view.renderer = nullptr;
		view.transform = nullptr;
		_views.erase(iterator);
	}

	void BattleUnitPresenter::_reconcile(const BattleSnapshot &p_snapshot, bool p_snap)
	{
		for (const BattleUnitSnapshot &unit : p_snapshot.units)
		{
			_createOrUpdate(unit, p_snap);
		}
		for (auto iterator = _views.begin(); iterator != _views.end();)
		{
			const bool exists = std::any_of(p_snapshot.units.begin(), p_snapshot.units.end(), [id = iterator->first](const BattleUnitSnapshot &unit) {
				return unit.id == id && shown(unit);
			});
			if (!exists)
			{
				const BattleUnitId id = iterator->first;
				++iterator;
				_remove(id);
			}
			else
			{
				++iterator;
			}
		}
		_snapshot = p_snapshot;
	}

	void BattleUnitPresenter::_startTrack(BattleUnitId p_unit, UnitCosmeticTrack p_track)
	{
		const auto iterator = _views.find(p_unit);
		if (iterator == _views.end() || p_track.path.size() < 2)
		{
			return;
		}
		BattleUnitView &view = iterator->second;
		if (view.cosmetic.has_value())
		{
			if (view.queuedCosmetics.size() >= 8)
			{
				view.cosmetic.reset();
				while (!view.queuedCosmetics.empty())
				{
					view.queuedCosmetics.pop();
				}
				return;
			}
			view.queuedCosmetics.push(std::move(p_track));
		}
		else
		{
			view.cosmetic = std::move(p_track);
		}
	}

	void BattleUnitPresenter::attach(
		spk::GameEngine &p_engine,
		const BattlePresentationBoardBinding &p_boardPresentation,
		const Registries &p_registries,
		const spk::Texture &p_texture,
		const BattleSnapshot &p_snapshot,
		BattleEventSequence p_nextSequence)
	{
		detach();
		_engine = &p_engine;
		_board = &p_boardPresentation;
		_registries = &p_registries;
		_texture = &p_texture;
		_cubeMesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(BattleUnitBaseEdge));
		_nextSequence = p_nextSequence;
		_reconcile(p_snapshot, true);
	}

	void BattleUnitPresenter::consume(std::span<const BattleEvent> p_events, const BattleSnapshot &p_afterBatch)
	{
		if (!attached())
		{
			return;
		}
		std::map<BattleUnitId, std::vector<BoardCell>> movementPaths;
		std::map<BattleUnitId, BattleActionId> movementActions;
		bool gap = false;
		for (const BattleEvent &event : p_events)
		{
			if (event.header.sequence.value < _nextSequence.value)
			{
				continue;
			}
			if (event.header.sequence.value != _nextSequence.value)
			{
				gap = true;
				break;
			}
			++_nextSequence.value;
			std::visit([&](const auto &payload) {
				using Payload = std::decay_t<decltype(payload)>;
				if constexpr (std::is_same_v<Payload, UnitMovementStep>)
				{
					auto &path = movementPaths[payload.unit];
					if (path.empty())
					{
						path.push_back(payload.from);
					}
					if (path.back() == payload.from)
					{
						path.push_back(payload.to);
					}
					else
					{
						path.clear();
					}
					movementActions[payload.unit] = event.header.action.value_or(BattleActionId{});
				}
				else if constexpr (std::is_same_v<Payload, UnitMoved>)
				{
					const auto path = movementPaths.find(payload.unit);
					if (path != movementPaths.end() && path->second.size() == static_cast<std::size_t>(payload.enteredCellCount) + 1U &&
						path->second.front() == payload.startCell && path->second.back() == payload.endCell)
					{
						_startTrack(payload.unit, {.action = movementActions[payload.unit], .path = path->second});
					}
				}
			},
					   event.payload);
		}
		_reconcile(p_afterBatch, gap);
	}

	void BattleUnitPresenter::update(const spk::UpdateContext &p_tick)
	{
		const float seconds = std::max(0.0f, static_cast<float>(p_tick.deltaTime.seconds()));
		for (auto &[id, view] : _views)
		{
			(void)id;
			if (!view.cosmetic.has_value())
			{
				continue;
			}
			UnitCosmeticTrack &track = *view.cosmetic;
			while (track.segment + 1U < track.path.size())
			{
				const BoardCell &from = track.path[track.segment];
				const BoardCell &to = track.path[track.segment + 1U];
				const float distance = std::max(0.001f, spk::Vector3(static_cast<float>(to.x - from.x), 0.0f, static_cast<float>(to.z - from.z)).length());
				track.segmentProgress += seconds * track.worldUnitsPerSecond / distance;
				if (track.segmentProgress < 1.0f)
				{
					view.transform->setPosition(battleUnitSegmentCenterPosition(_board->board, from, to, track.segmentProgress, view.renderedHeight));
					break;
				}
				++track.segment;
				track.segmentProgress = 0.0f;
			}
			if (track.segment + 1U >= track.path.size())
			{
				view.transform->setPosition(battleUnitCenterPosition(_board->board, view.authoritativeCell, view.renderedHeight));
				view.cosmetic.reset();
				if (!view.queuedCosmetics.empty())
				{
					view.cosmetic = std::move(view.queuedCosmetics.front());
					view.queuedCosmetics.pop();
				}
			}
		}
	}

	void BattleUnitPresenter::fastForward()
	{
		for (auto &[id, view] : _views)
		{
			(void)id;
			view.cosmetic.reset();
			while (!view.queuedCosmetics.empty())
			{
				view.queuedCosmetics.pop();
			}
			view.transform->setPosition(battleUnitCenterPosition(_board->board, view.authoritativeCell, view.renderedHeight));
		}
	}

	void BattleUnitPresenter::detach() noexcept
	{
		for (auto iterator = _views.begin(); iterator != _views.end();)
		{
			const BattleUnitId id = iterator->first;
			++iterator;
			_remove(id);
		}
		_cubeMesh.reset();
		_snapshot = {};
		_nextSequence = {};
		_texture = nullptr;
		_registries = nullptr;
		_board = nullptr;
		_engine = nullptr;
	}

	bool BattleUnitPresenter::attached() const noexcept
	{
		return _engine != nullptr;
	}
	std::size_t BattleUnitPresenter::viewCount() const noexcept
	{
		return _views.size();
	}
	BattleEventSequence BattleUnitPresenter::nextSequence() const noexcept
	{
		return _nextSequence;
	}
}
