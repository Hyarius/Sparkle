#include "battle/presentation/battle_object_presenter.hpp"

#include "battle/presentation/battle_presentation_cell_source.hpp"
#include "battle/presentation/battle_unit_position.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/system/event/spk_update_context.hpp"

#include <algorithm>
#include <type_traits>

namespace pg
{
	void BattleObjectPresenter::_createOrUpdate(const BattleObjectSnapshot &p_object)
	{
		auto [iterator, inserted] = _views.try_emplace(p_object.id);
		BattleObjectView &view = iterator->second;
		if (inserted)
		{
			view.id = p_object.id;
			view.entity = std::make_unique<spk::Entity3D>();
			view.transform = &view.entity->transform();
			view.renderer = &view.entity->addComponent<spk::TextureMeshRenderer3D>();
			view.renderer->setMesh(_mesh);
			view.renderer->setTexture(_texture);
			view.renderer->setCastsShadows(false);
			view.renderer->setReceivesShadows(false);
			_engine->addEntity(view.entity.get());
		}
		view.authoritativeCell = p_object.cell;
		view.creatorSide = p_object.creator.has_value() ? std::optional<BattleSide>(p_object.creatorSide) : std::nullopt;
		const spk::Color tint = p_object.creatorSide == BattleSide::Player
									? spk::Color{0.28f, 0.72f, 1.0f, 1.0f}
									: spk::Color{1.0f, 0.42f, 0.32f, 1.0f};
		view.renderer->setTint(tint);
		const float scale = p_object.blocksMovement ? 0.34f : 0.20f;
		view.transform->setScale({scale, scale, scale});
		spk::Vector3 position = battleUnitCenterPosition(_board->board, p_object.cell, scale);
		position.x += 0.24f;
		position.z += 0.24f;
		view.transform->setPosition(position);
	}

	void BattleObjectPresenter::_remove(BattleObjectId p_id) noexcept
	{
		const auto iterator = _views.find(p_id);
		if (iterator == _views.end())
		{
			return;
		}
		BattleObjectView &view = iterator->second;
		view.cosmetic.reset();
		if (_engine != nullptr && view.entity != nullptr)
		{
			_engine->removeEntity(view.entity.get());
		}
		view.renderer = nullptr;
		view.transform = nullptr;
		_views.erase(iterator);
	}

	void BattleObjectPresenter::_reconcile(const BattleSnapshot &p_snapshot)
	{
		for (const BattleObjectSnapshot &object : p_snapshot.objects)
		{
			_createOrUpdate(object);
		}
		for (auto iterator = _views.begin(); iterator != _views.end();)
		{
			const bool exists = std::any_of(p_snapshot.objects.begin(), p_snapshot.objects.end(), [id = iterator->first](const BattleObjectSnapshot &object) {
				return object.id == id;
			});
			if (!exists)
			{
				// A trigger and removal may share one committed batch. Keep the small marker until
				// its bounded cosmetic cue completes, then remove it in update(); it is never put
				// back into the authoritative object projection.
				if (iterator->second.cosmetic.has_value() && iterator->second.cosmetic->removeWhenComplete)
				{
					++iterator;
					continue;
				}
				const BattleObjectId id = iterator->first;
				++iterator;
				_remove(id);
			}
			else
			{
				++iterator;
			}
		}
	}

	void BattleObjectPresenter::attach(
		spk::GameEngine &p_engine,
		const BattlePresentationBoardBinding &p_boardPresentation,
		const spk::Texture &p_texture,
		const BattleSnapshot &p_snapshot,
		BattleEventSequence p_nextSequence)
	{
		detach();
		_engine = &p_engine;
		_board = &p_boardPresentation;
		_texture = &p_texture;
		_mesh = std::make_shared<spk::TextureMesh3D>(spk::PrimitiveObject::CreateCube(1.0f));
		_nextSequence = p_nextSequence;
		_reconcile(p_snapshot);
	}

	void BattleObjectPresenter::consume(std::span<const BattleEvent> p_events, const BattleSnapshot &p_afterBatch)
	{
		if (_engine == nullptr)
		{
			return;
		}
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
				if constexpr (std::is_same_v<Payload, BattleObjectTriggered>)
				{
					if (const auto iterator = _views.find(payload.object); iterator != _views.end())
					{
						iterator->second.cosmetic = ObjectCosmeticCue{.secondsRemaining = 0.18f};
					}
				}
				else if constexpr (std::is_same_v<Payload, BattleObjectRemoved>)
				{
					if (const auto iterator = _views.find(payload.object); iterator != _views.end())
					{
						iterator->second.cosmetic = ObjectCosmeticCue{.secondsRemaining = 0.18f, .removeWhenComplete = true};
					}
				}
			},
					   event.payload);
		}
		(void)gap;
		_reconcile(p_afterBatch);
	}

	void BattleObjectPresenter::update(const spk::UpdateContext &p_tick)
	{
		const float delta = std::max(0.0f, static_cast<float>(p_tick.deltaTime.seconds()));
		for (auto iterator = _views.begin(); iterator != _views.end();)
		{
			BattleObjectView &view = iterator->second;
			if (!view.cosmetic.has_value())
			{
				++iterator;
				continue;
			}
			view.cosmetic->secondsRemaining -= delta;
			if (view.cosmetic->secondsRemaining > 0.0f)
			{
				++iterator;
				continue;
			}
			if (view.cosmetic->removeWhenComplete)
			{
				const BattleObjectId id = iterator->first;
				++iterator;
				_remove(id);
			}
			else
			{
				view.cosmetic.reset();
				++iterator;
			}
		}
	}

	void BattleObjectPresenter::fastForward()
	{
		for (auto &[id, view] : _views)
		{
			(void)id;
			view.cosmetic.reset();
		}
	}

	void BattleObjectPresenter::detach() noexcept
	{
		for (auto iterator = _views.begin(); iterator != _views.end();)
		{
			const BattleObjectId id = iterator->first;
			++iterator;
			_remove(id);
		}
		_mesh.reset();
		_nextSequence = {};
		_texture = nullptr;
		_board = nullptr;
		_engine = nullptr;
	}

	std::size_t BattleObjectPresenter::viewCount() const noexcept
	{
		return _views.size();
	}
}
