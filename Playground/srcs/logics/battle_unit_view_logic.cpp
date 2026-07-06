#include "logics/battle_unit_view_logic.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "board/cell_source.hpp"
#include "board/pathfinder.hpp"
#include "core/event_center.hpp"
#include "core/registry.hpp"
#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"

#include <algorithm>

namespace pg
{
	BattleUnitViewLogic::BattleUnitViewLogic(
		spk::GameEngine &p_engine,
		const spk::Texture *p_texture,
		const Registry<ModelDefinition> *p_models) :
		_engine(p_engine),
		_texture(p_texture),
		_models(p_models),
		_fallbackModel(ModelDefinition::placeholderCube())
	{
	}

	BattleUnitViewLogic::~BattleUnitViewLogic()
	{
		end();
	}

	void BattleUnitViewLogic::begin(BattleContext &p_context)
	{
		end();
		_context = &p_context;
		_eventContract = p_context.events().battleEventOccurred.subscribe([this](const BattleEvent *p_event) {
			_onBattleEvent(p_event);
		});
		_placementContract = p_context.events().battleUnitPlaced.subscribe([this](BattleUnit *p_unit) {
			if (p_unit != nullptr && p_unit->boardPosition.has_value() && !_entries.contains(p_unit))
			{
				_spawn(*p_unit, *p_unit->boardPosition);
			}
		});
		_placementChangedContract = p_context.events().battlePlacementChanged.subscribe([this] {
			synchronize();
		});
		_impressedContract = p_context.events().creatureImpressed.subscribe([this](BattleUnit *p_unit) {
			if (p_unit != nullptr)
			{
				_remove(*p_unit);
			}
		});
		synchronize();
	}

	void BattleUnitViewLogic::_spawn(BattleUnit &p_unit, const spk::Vector3Int &p_cell)
	{
		auto entity = std::make_unique<spk::Entity3D>();
		BattleUnitView &view = entity->addComponent<BattleUnitView>(p_unit, p_cell);
		const CreatureUnit *source = p_unit.source();
		const CreatureForm &form = source->species->form(source->currentFormId);
		const ModelDefinition *model = _models != nullptr ? _models->tryGet(form.modelId) : nullptr;
		if (model == nullptr)
		{
			model = &_fallbackModel;
		}
		std::vector<std::unique_ptr<spk::Entity3D>> partEntities;
		partEntities.reserve(model->parts.size());
		for (const ModelPart &part : model->parts)
		{
			auto partEntity = std::make_unique<spk::Entity3D>(entity.get());
			partEntity->transform().setPosition({-part.origin.x, -part.origin.y, -part.origin.z});
			spk::TextureMeshRenderer3D &renderer = partEntity->addComponent<spk::TextureMeshRenderer3D>();
			renderer.setMesh(part.mesh);
			renderer.setTexture(_texture);
			renderer.setTint(
				p_unit.side == BattleSide::Player
					? spk::Color{0.25f, 0.65f, 1.0f, 1.0f}
					: spk::Color{1.0f, 0.28f, 0.22f, 1.0f});
			partEntities.push_back(std::move(partEntity));
		}
		_place(view, {static_cast<float>(p_cell.x) + 0.5f, walkHeightAtCenter(_context->board.terrain().cells(), p_cell), static_cast<float>(p_cell.z) + 0.5f});
		_engine.addEntity(entity.get());
		for (const auto &partEntity : partEntities)
		{
			_engine.addEntity(partEntity.get());
		}
		_entries.emplace(&p_unit, Entry{std::move(entity), std::move(partEntities), &view});
	}

	void BattleUnitViewLogic::synchronize()
	{
		if (_context == nullptr)
		{
			return;
		}
		std::vector<BattleUnit *> removed;
		for (const auto &[unit, entry] : _entries)
		{
			(void)entry;
			if (unit == nullptr || !unit->boardPosition.has_value()) removed.push_back(unit);
		}
		for (BattleUnit *unit : removed)
		{
			if (unit != nullptr) _remove(*unit);
		}
		for (BattleUnit *unit : _context->allUnits())
		{
			if (unit == nullptr || !unit->boardPosition.has_value() || unit->isDefeated()) continue;
			const auto found = _entries.find(unit);
			if (found == _entries.end())
			{
				_spawn(*unit, *unit->boardPosition);
				continue;
			}
			BattleUnitView &view = *found->second.view;
			view.displayedCell = *unit->boardPosition;
			view.path.clear();
			_place(view, {
				static_cast<float>(unit->boardPosition->x) + 0.5f,
				walkHeightAtCenter(_context->board.terrain().cells(), *unit->boardPosition),
				static_cast<float>(unit->boardPosition->z) + 0.5f});
		}
	}

	void BattleUnitViewLogic::_place(BattleUnitView &p_view, const spk::Vector3 &p_localPosition)
	{
		if (_context == nullptr || p_view.entity() == nullptr)
		{
			return;
		}
		const spk::Vector3Int anchor = _context->board.worldAnchor();
		const spk::Vector3 worldPosition = p_localPosition + spk::Vector3{
																 static_cast<float>(anchor.x),
																 static_cast<float>(anchor.y) + 0.5f,
																 static_cast<float>(anchor.z)};
		p_view.entity()->requireComponent<spk::Transform3D>().setPosition(worldPosition);
	}

	void BattleUnitViewLogic::_beginMove(BattleUnit &p_unit, const spk::Vector3Int &p_destination)
	{
		const auto found = _entries.find(&p_unit);
		if (_context == nullptr || found == _entries.end() || found->second.view->sinking)
		{
			return;
		}
		BattleUnitView &view = *found->second.view;
		const auto path = Pathfinder::findPath(
			_context->board.navigation().graph(), view.displayedCell, p_destination);
		if (!path.has_value() || path->size() < 2)
		{
			view.displayedCell = p_destination;
			view.path.clear();
			_place(view, {static_cast<float>(p_destination.x) + 0.5f, walkHeightAtCenter(_context->board.terrain().cells(), p_destination), static_cast<float>(p_destination.z) + 0.5f});
			return;
		}
		view.path = *path;
		view.segment = 0;
		view.segmentProgress = 0.0f;
	}

	void BattleUnitViewLogic::_beginDefeat(BattleUnit &p_unit)
	{
		const auto found = _entries.find(&p_unit);
		if (found == _entries.end())
		{
			return;
		}
		BattleUnitView &view = *found->second.view;
		view.path.clear();
		view.segment = 0;
		view.segmentProgress = 0.0f;
		view.sinking = true;
		view.sinkSeconds = 0.0f;
	}

	void BattleUnitViewLogic::_remove(BattleUnit &p_unit)
	{
		const auto found = _entries.find(&p_unit);
		if (found == _entries.end())
		{
			return;
		}
		for (const auto &partEntity : found->second.partEntities)
		{
			_engine.removeEntity(partEntity.get());
		}
		_engine.removeEntity(found->second.entity.get());
		_entries.erase(found);
	}

	void BattleUnitViewLogic::_onBattleEvent(const BattleEvent *p_event)
	{
		if (p_event == nullptr)
		{
			return;
		}
		if (const DistanceTravelledEvent *travelled = p_event->getIf<DistanceTravelledEvent>();
			travelled != nullptr && travelled->context.caster != nullptr && travelled->context.caster->boardPosition.has_value())
		{
			_beginMove(*travelled->context.caster, *travelled->context.caster->boardPosition);
		}
		else if (const UnitDefeatedEvent *defeated = p_event->getIf<UnitDefeatedEvent>();
			defeated != nullptr && defeated->context.target != nullptr)
		{
			_beginDefeat(*defeated->context.target);
		}
	}

	void BattleUnitViewLogic::advance(float p_seconds)
	{
		if (_context == nullptr)
		{
			return;
		}
		const float seconds = std::max(0.0f, p_seconds);
		for (auto &[unit, entry] : _entries)
		{
			(void)unit;
			BattleUnitView &view = *entry.view;
			if (view.sinking && !view.hidden)
			{
				view.sinkSeconds += seconds;
				spk::Transform3D &transform = entry.entity->transform();
				transform.move({0.0f, -seconds * 1.5f, 0.0f});
				if (view.sinkSeconds >= SinkDuration)
				{
					view.hidden = true;
					entry.entity->deactivate();
				}
				continue;
			}

			float remaining = seconds * MoveSpeed;
			while (remaining > 0.0f && view.moving())
			{
				const spk::Vector3Int from = view.path[view.segment];
				const spk::Vector3Int to = view.path[view.segment + 1];
				const spk::Vector3 start = interpolateWalkSegment(_context->board.terrain().cells(), from, to, 0.0f);
				const spk::Vector3 finish = interpolateWalkSegment(_context->board.terrain().cells(), from, to, 1.0f);
				const float length = std::max(1.0f, start.distance(finish));
				const float available = (1.0f - view.segmentProgress) * length;
				const float consumed = std::min(remaining, available);
				view.segmentProgress += consumed / length;
				remaining -= consumed;
				_place(view, interpolateWalkSegment(_context->board.terrain().cells(), from, to, view.segmentProgress));
				if (view.segmentProgress >= 1.0f - 0.0001f)
				{
					view.displayedCell = to;
					++view.segment;
					view.segmentProgress = 0.0f;
				}
			}
			if (!view.moving())
			{
				view.path.clear();
				view.segment = 0;
			}
		}
	}

	void BattleUnitViewLogic::end()
	{
		_eventContract.resign();
		_placementContract.resign();
		_placementChangedContract.resign();
		_impressedContract.resign();
		for (auto &[unit, entry] : _entries)
		{
			(void)unit;
			for (const auto &partEntity : entry.partEntities)
			{
				_engine.removeEntity(partEntity.get());
			}
			_engine.removeEntity(entry.entity.get());
		}
		_entries.clear();
		_context = nullptr;
	}

	bool BattleUnitViewLogic::viewBusy() const noexcept
	{
		return std::ranges::any_of(_entries, [](const auto &p_entry) {
			return p_entry.second.view->moving();
		});
	}

	std::size_t BattleUnitViewLogic::registeredViewCount() const noexcept
	{
		return _entries.size();
	}

	std::size_t BattleUnitViewLogic::activeViewCount() const noexcept
	{
		return static_cast<std::size_t>(std::ranges::count_if(_entries, [](const auto &p_entry) {
			const BattleUnitView &view = *p_entry.second.view;
			return !view.sinking && !view.hidden;
		}));
	}

	const BattleUnitView *BattleUnitViewLogic::find(const BattleUnit &p_unit) const noexcept
	{
		const auto found = _entries.find(const_cast<BattleUnit *>(&p_unit));
		return found == _entries.end() ? nullptr : found->second.view;
	}

	std::size_t BattleUnitViewLogic::partCount(const BattleUnit &p_unit) const noexcept
	{
		const auto found = _entries.find(const_cast<BattleUnit *>(&p_unit));
		return found == _entries.end() ? 0 : found->second.partEntities.size();
	}

	void BattleUnitViewLogic::_executeUpdate(const spk::UpdateTick &p_tick)
	{
		advance(static_cast<float>(p_tick.deltaTime.seconds()));
	}
}
