#include "logics/battle_unit_view_logic.hpp"

#include "battle/battle_context.hpp"
#include "battle/battle_unit.hpp"
#include "board/cell_source.hpp"
#include "board/pathfinder.hpp"
#include "core/event_center.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"

#include <algorithm>

namespace pg
{
	BattleUnitViewLogic::BattleUnitViewLogic(spk::GameEngine &p_engine, const spk::Texture *p_texture) :
		_engine(p_engine),
		_texture(p_texture),
		_mesh(std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(0.8f)))
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
		synchronize();
	}

	void BattleUnitViewLogic::_spawn(BattleUnit &p_unit, const spk::Vector3Int &p_cell)
	{
		auto entity = std::make_unique<spk::Entity3D>();
		BattleUnitView &view = entity->addComponent<BattleUnitView>(p_unit, p_cell);
		spk::TextureMeshRenderer3D &renderer = entity->addComponent<spk::TextureMeshRenderer3D>();
		renderer.setMesh(_mesh);
		renderer.setTexture(_texture);
		renderer.setTint(
			p_unit.side == BattleSide::Player
				? spk::Color{0.25f, 0.65f, 1.0f, 1.0f}
				: spk::Color{1.0f, 0.28f, 0.22f, 1.0f});
		_place(view, {static_cast<float>(p_cell.x) + 0.5f, walkHeightAtCenter(_context->board.terrain().cells(), p_cell), static_cast<float>(p_cell.z) + 0.5f});
		_engine.addEntity(entity.get());
		_entries.emplace(&p_unit, Entry{std::move(entity), &view});
	}

	void BattleUnitViewLogic::synchronize()
	{
		if (_context == nullptr)
		{
			return;
		}
		for (BattleUnit *unit : _context->allUnits())
		{
			if (unit != nullptr && unit->boardPosition.has_value() && !unit->isDefeated() && !_entries.contains(unit))
			{
				_spawn(*unit, *unit->boardPosition);
			}
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

	void BattleUnitViewLogic::_onBattleEvent(const BattleEvent *p_event)
	{
		if (p_event == nullptr)
		{
			return;
		}
		if (p_event->type == BattleEventType::DistanceTravelled &&
			p_event->caster != nullptr && p_event->caster->boardPosition.has_value())
		{
			_beginMove(*p_event->caster, *p_event->caster->boardPosition);
		}
		else if (p_event->type == BattleEventType::UnitDefeated && p_event->target != nullptr)
		{
			_beginDefeat(*p_event->target);
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
		for (auto &[unit, entry] : _entries)
		{
			(void)unit;
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

	void BattleUnitViewLogic::_executeUpdate(const spk::UpdateTick &p_tick)
	{
		advance(static_cast<float>(p_tick.deltaTime.seconds()));
	}
}
