#include "core/mode_manager.hpp"

#include "battle/battle_pump.hpp"
#include "battle/battle_session.hpp"
#include "battle/presentation/battle_presentation_cell_source.hpp"
#include "battle/presentation/battle_presentation_runtime.hpp"
#include "board/board_builder.hpp"
#include "components/actor.hpp"
#include "core/game_context.hpp"
#include "core/registries.hpp"
#include "creatures/creature_unit.hpp"
#include "logics/actor_path_logic.hpp"
#include "logics/day_time_management_logic.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "structures/game_engine/spk_game_engine.hpp"
#include "structures/game_engine/spk_texture_mesh_renderer_3d.hpp"
#include "structures/graphics/geometry/spk_primitive_object.hpp"
#include "structures/graphics/geometry/spk_texture_mesh_3d.hpp"
#include "structures/voxel/spk_voxel_chunk.hpp"
#include "structures/voxel/spk_voxel_chunk_streamer.hpp"
#include "structures/voxel/spk_voxel_fluid_simulator.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <array>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace pg
{
	class ModeManager::BattleModeRuntime
	{
	private:
		BattleStartRequest _request;
		std::unique_ptr<BattleSession> _session;
		spk::VoxelFluidSimulator &_fluid;
		DayTimeManagementLogic &_dayTimeLogic;
		spk::TextureMeshRenderer3D &_playerRenderer;
		spk::TextureMeshRenderer3D &_hoverRenderer;
		spk::VoxelChunkStreamer &_playerStreamer;
		spk::VoxelChunkStreamer &_battleStreamer;
		VoxelWorld &_world;
		spk::GameEngine &_engine;
		const spk::Texture &_voxelTexture;
		std::shared_ptr<const spk::TextureMesh3D> _hoverMesh;
		bool _fluidWasActive = false;
		bool _dayTimeWasActive = false;
		bool _playerRendererWasActive = false;
		bool _hoverRendererWasActive = false;
		bool _playerStreamerWasActive = false;
		bool _battleStreamerWasActive = false;
		spk::Vector3Int _battleStreamerOrigin{};
		spk::Vector3Int _battleStreamerRange{};
		std::vector<std::pair<spk::Vector3Int, bool>> _worldChunkStates;
		std::unique_ptr<spk::Entity3D> _arenaEntity;
		std::unique_ptr<BoardPresentationCellSource> _presentationCells;
		std::unique_ptr<BattlePresentationBoardBinding> _presentationBinding;
		std::unique_ptr<BattlePresentationRuntime> _presentation;
		bool _shutdown = false;
		BattlePump _pump;

		[[nodiscard]] static std::shared_ptr<spk::TextureMesh3D> _buildArenaMesh(const PrefabDefinition &p_prefab)
		{
			const spk::TextureMesh3D cube = spk::PrimitiveObject::CreateCube(1.0f);
			const std::span<const spk::TextureVertex3D> cubeVertices = cube.vertices();
			spk::TextureMesh3D::Builder builder;
			std::size_t solidCount = 0;
			p_prefab.prefab.forEachAppliedVoxel(
				p_prefab.prefab.pivot(),
				VoxelOrientation::PositiveZ,
				[&solidCount](const spk::Vector3Int &, const spk::VoxelCell &p_cell) {
					if (!p_cell.isEmpty())
					{
						++solidCount;
					}
				});
			builder.reserve(solidCount * 24U, solidCount * 36U);
			p_prefab.prefab.forEachAppliedVoxel(
				p_prefab.prefab.pivot(),
				VoxelOrientation::PositiveZ,
				[&builder, &cubeVertices](const spk::Vector3Int &p_cell, const spk::VoxelCell &p_voxel) {
					if (p_voxel.isEmpty())
					{
						return;
					}
					const spk::Vector3 offset = spk::Vector3(p_cell) + spk::Vector3{0.5f, 0.5f, 0.5f};
					for (std::size_t face = 0; face < 6; ++face)
					{
						std::array<spk::TextureVertex3D, 4> vertices;
						for (std::size_t index = 0; index < 4; ++index)
						{
							vertices[index] = cubeVertices[face * 4 + index];
							vertices[index].position += offset;
						}
						builder.addShape(vertices[0], vertices[1], vertices[2], vertices[3]);
					}
				});
			return std::make_shared<spk::TextureMesh3D>(builder.bake());
		}

		void _activateHandcraftedArena(const PrefabDefinition &p_prefab)
		{
			const std::shared_ptr<spk::TextureMesh3D> mesh = _buildArenaMesh(p_prefab);
			// The generated map remains owned and intact, but every currently rendered chunk is
			// suspended exactly as found.  The player streamer is paused for this isolated source and
			// restored on exit; it never receives handcrafted coordinates.
			_playerStreamerWasActive = _playerStreamer.isActivated();
			_world.map().forEachLoadedChunk([this](const spk::VoxelChunk &p_chunk) {
				_worldChunkStates.emplace_back(p_chunk.coordinates(), p_chunk.isActivated());
			});
			if (_playerStreamerWasActive)
			{
				_playerStreamer.deactivate();
			}
			for (const auto &[coordinates, wasActive] : _worldChunkStates)
			{
				if (wasActive)
				{
					_world.map().setChunkActive(coordinates, false);
				}
			}

			try
			{
				_arenaEntity = std::make_unique<spk::Entity3D>();
				auto &renderer = _arenaEntity->addComponent<spk::TextureMeshRenderer3D>();
				renderer.setMesh(mesh);
				renderer.setTexture(&_voxelTexture);
				renderer.setTint({0.96f, 0.96f, 0.96f, 1.0f});
				_engine.addEntity(_arenaEntity.get());
			} catch (...)
			{
				if (_arenaEntity != nullptr)
				{
					_engine.removeEntity(_arenaEntity.get());
					_arenaEntity.reset();
				}
				for (const auto &[coordinates, wasActive] : _worldChunkStates)
				{
					if (wasActive)
					{
						_world.map().setChunkActive(coordinates, true);
					}
				}
				_worldChunkStates.clear();
				if (_playerStreamerWasActive)
				{
					_playerStreamer.activate();
				}
				throw;
			}
		}

	public:
		BattleModeRuntime(
			BattleStartRequest p_request,
			std::unique_ptr<BattleSession> p_session,
			spk::VoxelFluidSimulator &p_fluid,
			bool p_fluidWasActive,
			DayTimeManagementLogic &p_dayTimeLogic,
			spk::TextureMeshRenderer3D &p_playerRenderer,
			spk::TextureMeshRenderer3D &p_hoverRenderer,
			spk::VoxelChunkStreamer &p_playerStreamer,
			spk::VoxelChunkStreamer &p_battleStreamer,
			bool p_battleStreamerWasActive,
			spk::Vector3Int p_battleStreamerOrigin,
			spk::Vector3Int p_battleStreamerRange,
			VoxelWorld &p_world,
			spk::GameEngine &p_engine,
			const spk::Texture &p_voxelTexture,
			spk::Camera3D &p_camera,
			const spk::Texture &p_maskTexture,
			BattleHudWidget &p_battleHud,
			std::function<spk::Vector2()> p_viewportSize,
			const GameRules &p_rules,
			const PrefabDefinition *p_handcraftedPrefab) :
			_request(std::move(p_request)),
			_session(std::move(p_session)),
			_fluid(p_fluid),
			_dayTimeLogic(p_dayTimeLogic),
			_playerRenderer(p_playerRenderer),
			_hoverRenderer(p_hoverRenderer),
			_playerStreamer(p_playerStreamer),
			_battleStreamer(p_battleStreamer),
			_world(p_world),
			_engine(p_engine),
			_voxelTexture(p_voxelTexture),
			_hoverMesh(_hoverRenderer.mesh()),
			_fluidWasActive(p_fluidWasActive),
			_dayTimeWasActive(p_dayTimeLogic.isActivated()),
			_playerRendererWasActive(p_playerRenderer.isActivated()),
			_hoverRendererWasActive(p_hoverRenderer.isActivated()),
			_battleStreamerWasActive(p_battleStreamerWasActive),
			_battleStreamerOrigin(p_battleStreamerOrigin),
			_battleStreamerRange(p_battleStreamerRange)
		{
			try
			{
				if (_session == nullptr)
				{
					throw std::invalid_argument("battle runtime requires a session");
				}
				// Battle owns a frozen tactical scene. Preserve the current sun/ambient state,
				// but stop the real-time day cycle until the battle runtime is torn down.
				if (_dayTimeWasActive)
				{
					_dayTimeLogic.deactivate();
				}
				if (_request.debugAutoplayPlayerProfileId.has_value())
				{
					_pump.setDebugPlayerAutoplayBehaviour(_request.debugAutoplayPlayerProfileId);
				}
				const std::shared_ptr<spk::TextureMesh3D> clearedHover = std::make_shared<spk::TextureMesh3D>();
				if (_session->board().sourceDescriptor().kind == BoardSourceKind::Handcrafted)
				{
					if (p_handcraftedPrefab == nullptr)
					{
						throw std::invalid_argument("handcrafted battle runtime requires its geometry prefab");
					}
					_activateHandcraftedArena(*p_handcraftedPrefab);
				}
				// Fluid was paused before live-board construction.  Suspension owns the other visual
				// state and leaves the player entity/streamer processable for the frozen world board.
				_hoverRenderer.setMesh(clearedHover);
				_hoverRenderer.deactivate();
				_playerRenderer.deactivate();
				_presentationCells = std::make_unique<BoardPresentationCellSource>(_session->board());
				_presentationBinding = std::make_unique<BattlePresentationBoardBinding>(BattlePresentationBoardBinding{
					.board = _session->board(),
					.cells = *_presentationCells,
					.bounds = presentationBounds(_session->board(), *_presentationCells),
					.source = _session->board().sourceDescriptor()});
				_presentation = std::make_unique<BattlePresentationRuntime>(
					_engine,
					p_camera,
					*_session,
					[this] {
						return _session->snapshot();
					},
					BattlePresentationCallbacks{},
					std::move(p_viewportSize),
					p_maskTexture,
					p_voxelTexture,
					p_rules,
					p_battleHud);
				_presentation->enter(*_presentationBinding);
			} catch (...)
			{
				// A failed presentation attach is an entry failure, not an orphaned arena, paused
				// fluid simulator, or hidden exploration renderer.
				shutdown();
				throw;
			}
		}

		~BattleModeRuntime()
		{
			shutdown();
		}

		[[nodiscard]] BattleSession &session() noexcept
		{
			return *_session;
		}
		[[nodiscard]] const BattleStartRequest &request() const noexcept
		{
			return _request;
		}

		void presentCommittedBatch(const CommittedBattleBatch &p_batch)
		{
			if (_presentation != nullptr)
			{
				_presentation->onCommittedBatch(p_batch);
			}
		}

		void update(const spk::UpdateContext &p_tick)
		{
			if (_session->phase() == BattlePhase::Deployment && _request.debugAutoplayPlayerProfileId.has_value())
			{
				const BattleSnapshot snapshot = _session->snapshot();
				for (const BattleUnitSnapshot &unit : snapshot.units)
				{
					if (unit.side != BattleSide::Player || unit.cell.has_value())
					{
						continue;
					}
					const auto cells = _session->board().deployment().cells(BattleSide::Player);
					for (const BoardCell &cell : cells)
					{
						if (!_session->board().occupancy().unitAt(cell).has_value())
						{
							(void)_session->submit(PlaceUnitCommand{unit.id, cell}, {CommandController::DebugAutoplay});
							if (_presentation != nullptr)
							{
								_presentation->update(p_tick);
							}
							return; // one public command per update, preserving ordinary batch boundaries
						}
					}
				}
				(void)_session->submit(ConfirmDeploymentCommand{}, {CommandController::DebugAutoplay});
				if (_presentation != nullptr)
				{
					_presentation->update(p_tick);
				}
				return;
			}
			(void)_pump.advanceUntilPlayerInput(*_session, 64);
			if (_presentation != nullptr)
			{
				_presentation->update(p_tick);
			}
		}

		[[nodiscard]] bool onMouseMoved(const spk::MouseMovedEvent &p_event)
		{
			return _presentation != nullptr && _presentation->onMouseMoved(p_event);
		}
		[[nodiscard]] bool onMousePressed(const spk::MouseButtonPressedEvent &p_event)
		{
			return _presentation != nullptr && _presentation->onMousePressed(p_event);
		}
		[[nodiscard]] bool onMouseReleased(const spk::MouseButtonReleasedEvent &p_event)
		{
			return _presentation != nullptr && _presentation->onMouseReleased(p_event);
		}
		[[nodiscard]] bool onMouseWheel(const spk::MouseWheelScrolledEvent &p_event)
		{
			return _presentation != nullptr && _presentation->onMouseWheel(p_event);
		}
		[[nodiscard]] bool onKeyPressed(const spk::KeyPressedEvent &p_event)
		{
			return _presentation != nullptr && _presentation->onKeyPressed(p_event);
		}
		void onMouseLeave() noexcept
		{
			if (_presentation != nullptr)
			{
				_presentation->onMouseLeave();
			}
		}

		void shutdown() noexcept
		{
			if (_shutdown)
			{
				return;
			}
			_shutdown = true;
			if (_presentation != nullptr)
			{
				_presentation->beginExit();
				_presentation->detach();
				_presentation.reset();
			}
			_presentationBinding.reset();
			_presentationCells.reset();
			// The session owns BoardData and releases it before either source-specific presentation
			// state (including an isolated arena) or fluid activity is restored.
			_session.reset();
			if (_arenaEntity != nullptr)
			{
				_engine.removeEntity(_arenaEntity.get());
				_arenaEntity.reset();
			}
			for (const auto &[coordinates, wasActive] : _worldChunkStates)
			{
				if (wasActive)
				{
					_world.map().setChunkActive(coordinates, true);
				}
			}
			_worldChunkStates.clear();
			if (_playerStreamerWasActive)
			{
				_playerStreamer.activate();
			}
			_battleStreamer.setOriginPosition(_battleStreamerOrigin);
			_battleStreamer.setViewRange(_battleStreamerRange);
			if (_battleStreamerWasActive)
			{
				_battleStreamer.activate();
			}
			else
			{
				_battleStreamer.deactivate();
			}
			_hoverRenderer.setMesh(_hoverMesh);
			if (_hoverRendererWasActive)
			{
				_hoverRenderer.activate();
			}
			if (_playerRendererWasActive)
			{
				_playerRenderer.activate();
			}
			if (_fluidWasActive)
			{
				_fluid.activate();
			}
			if (_dayTimeWasActive)
			{
				_dayTimeLogic.activate();
			}
			else
			{
				_dayTimeLogic.deactivate();
			}
		}
	};

	namespace
	{
		[[nodiscard]] BattleParticipantSeed playerSeed(const CreatureUnit &p_creature, std::uint32_t p_order)
		{
			return {
				.side = BattleSide::Player,
				.rosterOrder = p_order,
				.persistentCreatureId = p_creature.id,
				.speciesId = p_creature.speciesId,
				.formId = p_creature.derived.formId,
				.attributes = p_creature.derived.attributes,
				.abilityIds = p_creature.derived.abilityIds,
				.passiveStatusIds = p_creature.derived.passiveStatusIds};
		}

		[[nodiscard]] BattleParticipantSeed enemySeed(
			const EncounterSpawnDefinition &p_spawn,
			std::uint32_t p_order,
			const Registries &p_registries)
		{
			const CreatureUnit projected = makeCreatureUnit(
				CreatureInstanceId::fromSerial(static_cast<std::uint64_t>(p_order) + 1U),
				p_spawn.speciesId,
				p_spawn.completedFeatNodeIds,
				p_registries);
			return {
				.side = BattleSide::Enemy,
				.rosterOrder = p_order,
				.encounterSpawnMemberId = p_spawn.id,
				.speciesId = projected.speciesId,
				.formId = projected.derived.formId,
				.attributes = projected.derived.attributes,
				.abilityIds = projected.derived.abilityIds,
				.passiveStatusIds = projected.derived.passiveStatusIds,
				.aiBehaviourId = p_spawn.aiBehaviourId,
				.inheritedCompletedFeatNodeIds = p_spawn.completedFeatNodeIds};
		}

		[[nodiscard]] WorldBoardRequest makeLiveBoardRequest(
			const BattleStartRequest &p_request,
			const ModeManager::Dependencies &p_dependencies,
			std::size_t p_playerSeats)
		{
			const auto &live = std::get<LiveWorldBoardPolicyDefinition>(p_request.encounter.board);
			const TraversalBounds &bounds = p_dependencies.worldNavigation.bounds();
			return {
				.encounterSupportCell = p_request.worldCell,
				.size = {live.size[0], live.size[1]},
				.minimumWorldY = bounds.minimum.y,
				.maximumWorldY = bounds.maximum.y,
				.approachDirection = p_request.playerApproach,
				.deploymentDepth = live.deploymentDepth,
				.playerSeatCount = p_playerSeats,
				.enemySeatCount = p_request.encounter.enemyRoster.size()};
		}

		struct PreparedBoard
		{
			BoardData board;
			std::optional<WorldBoardPlan> livePlan;
		};

		[[nodiscard]] PreparedBoard buildBoard(
			const BattleStartRequest &p_request,
			const ModeManager::Dependencies &p_dependencies,
			std::size_t p_playerSeats)
		{
			const std::size_t enemySeats = p_request.encounter.enemyRoster.size();
			if (const auto *live = std::get_if<LiveWorldBoardPolicyDefinition>(&p_request.encounter.board))
			{
				WorldBoardRequest request = makeLiveBoardRequest(p_request, p_dependencies, p_playerSeats);
				WorldBoardPlanResult plan = BoardBuilder::planWorld(request);
				if (!plan.plan.has_value())
				{
					throw std::runtime_error("unable to plan live-world battle board");
				}
				BoardBuildResult built = BoardBuilder::buildWorld(
					p_dependencies.world,
					*plan.plan,
					p_dependencies.registries.gameRules().maxVerticalTraversalGap);
				if (!built.board.has_value())
				{
					throw std::runtime_error("unable to build live-world battle board");
				}
				return {.board = std::move(*built.board), .livePlan = std::move(*plan.plan)};
			}

			const auto &handcrafted = std::get<HandcraftedBoardPolicyDefinition>(p_request.encounter.board);
			const HandcraftedBattleBoardDefinition *definition =
				p_dependencies.registries.battleBoards().tryGet(handcrafted.definitionId);
			if (definition == nullptr)
			{
				throw std::runtime_error("unknown handcrafted battle board");
			}
			const PrefabDefinition *prefab =
				p_dependencies.registries.prefabs().tryGet(definition->geometryPrefabId);
			if (prefab == nullptr)
			{
				throw std::runtime_error("unknown handcrafted battle board prefab");
			}
			BoardBuildResult built = BoardBuilder::buildHandcrafted(
				*definition,
				*prefab,
				p_dependencies.registries.voxels(),
				p_playerSeats,
				enemySeats,
				p_dependencies.registries.gameRules().maxVerticalTraversalGap);
			if (!built.board.has_value())
			{
				throw std::runtime_error("unable to build handcrafted battle board");
			}
			return {.board = std::move(*built.board), .livePlan = std::nullopt};
		}
	}

	ModeManager::ModeManager(Dependencies p_dependencies) :
		_dependencies(p_dependencies)
	{
	}

	ModeManager::~ModeManager()
	{
		shutdown();
	}

	GameModeKind ModeManager::mode() const noexcept
	{
		return _dependencies.game.control.mode.value();
	}

	bool ModeManager::hasPendingTransition() const noexcept
	{
		return _stagedLiveEntry.has_value() || !std::holds_alternative<std::monostate>(_pending);
	}

	BattleSession *ModeManager::activeBattle() noexcept
	{
		return _battle == nullptr ? nullptr : &_battle->session();
	}

	const BattleSession *ModeManager::activeBattle() const noexcept
	{
		return _battle == nullptr ? nullptr : &_battle->session();
	}

	BattleId ModeManager::activeBattleId() const noexcept
	{
		return _battle == nullptr ? BattleId{} : _battle->session().battleId();
	}

	BattleRuntimeGeneration ModeManager::_allocateGeneration()
	{
		if (!_nextGeneration.valid())
		{
			throw std::overflow_error("battle runtime generation space is exhausted");
		}
		const BattleRuntimeGeneration result = _nextGeneration;
		if (_nextGeneration.value == std::numeric_limits<std::uint64_t>::max())
		{
			_nextGeneration.value = 0;
		}
		else
		{
			++_nextGeneration.value;
		}
		return result;
	}

	bool ModeManager::requestBattle(BattleStartRequest p_request)
	{
		if (mode() != GameModeKind::Exploration || _battle != nullptr || hasPendingTransition())
		{
			return false;
		}
		_pending = std::move(p_request);
		return true;
	}

	bool ModeManager::requestBattleExit(BattleTerminalRecord p_record)
	{
		if (_battle == nullptr || hasPendingTransition())
		{
			return false;
		}
		_pending = std::move(p_record);
		return true;
	}

	void ModeManager::_publishBatches()
	{
		if (_battle == nullptr)
		{
			return;
		}
		BattleSession &session = _battle->session();
		for (CommittedBattleBatch &batch : session.takeCommittedBatches())
		{
			_battle->presentCommittedBatch(batch);
			_dependencies.game.events.battleBatchPublished.trigger(
				{.generation = _activeGeneration, .battleId = session.battleId(), .batch = std::move(batch)});
		}
	}

	void ModeManager::_enter(
		BattleStartRequest p_request,
		BattleRuntimeGeneration p_generation,
		std::optional<BattleStreamerState> p_stagedStreamer)
	{
		const BattleId battleId = deriveBattleId(p_request.combatSeed);
		const std::string encounterId = p_request.encounter.encounterDefinitionId;
		try
		{
			if (p_request.stableEncounterInstanceId.empty() || p_request.encounter.enemyRoster.empty())
			{
				throw std::invalid_argument("battle request has no stable identity or enemies");
			}
			std::vector<BattleParticipantSeed> participants;
			for (std::size_t slot = 0; slot < PlayerRoster::TeamCapacity; ++slot)
			{
				const std::optional<CreatureUnit> &member = _dependencies.game.player.roster.team()[slot];
				if (member.has_value())
				{
					participants.push_back(playerSeed(*member, static_cast<std::uint32_t>(slot)));
				}
			}
			if (participants.empty())
			{
				throw std::invalid_argument("player roster has no active creatures");
			}
			for (std::size_t index = 0; index < p_request.encounter.enemyRoster.size(); ++index)
			{
				participants.push_back(enemySeed(
					p_request.encounter.enemyRoster[index],
					static_cast<std::uint32_t>(index),
					_dependencies.registries));
			}
			if (std::holds_alternative<LiveWorldBoardPolicyDefinition>(p_request.encounter.board) &&
				!p_stagedStreamer.has_value())
			{
				const WorldBoardPlanResult plan = BoardBuilder::planWorld(
					makeLiveBoardRequest(p_request, _dependencies, participants.size() - p_request.encounter.enemyRoster.size()));
				if (!plan.plan.has_value())
				{
					throw std::runtime_error("unable to plan live-world battle board");
				}
				BattleStreamerState state{
					.wasActive = _dependencies.battleStreamer.isActivated(),
					.origin = _dependencies.battleStreamer.originPosition(),
					.range = _dependencies.battleStreamer.viewRange()};
				_dependencies.battleStreamer.setOriginPosition(plan.plan->pinWindowOriginChunk);
				_dependencies.battleStreamer.setViewRange(plan.plan->pinWindowRange);
				_dependencies.battleStreamer.activate();
				_stagedLiveEntry = StagedLiveEntry{
					.request = std::move(p_request),
					.generation = p_generation,
					.streamer = state};
				return;
			}

			// Live board construction reads frozen terrain.  Pausing first makes a board graph and its
			// required-chunk revision stamps stable for the entire transaction, including failures.
			const bool fluidWasActive = _dependencies.fluidSimulator.isActivated();
			const BattleStreamerState battleStreamerState = p_stagedStreamer.value_or(BattleStreamerState{.wasActive = _dependencies.battleStreamer.isActivated(), .origin = _dependencies.battleStreamer.originPosition(), .range = _dependencies.battleStreamer.viewRange()});
			if (fluidWasActive)
			{
				_dependencies.fluidSimulator.deactivate();
			}
			try
			{
				PreparedBoard prepared =
					buildBoard(p_request, _dependencies, participants.size() - p_request.encounter.enemyRoster.size());
				if (prepared.livePlan.has_value())
				{
					_dependencies.battleStreamer.setOriginPosition(prepared.livePlan->pinWindowOriginChunk);
					_dependencies.battleStreamer.setViewRange(prepared.livePlan->pinWindowRange);
					_dependencies.battleStreamer.activate();
				}
				BoardData board = std::move(prepared.board);
				const PrefabDefinition *handcraftedPrefab = nullptr;
				if (const auto *handcrafted =
						std::get_if<HandcraftedBoardPolicyDefinition>(&p_request.encounter.board))
				{
					const HandcraftedBattleBoardDefinition &boardDefinition =
						_dependencies.registries.battleBoards().get(handcrafted->definitionId);
					handcraftedPrefab =
						_dependencies.registries.prefabs().tryGet(boardDefinition.geometryPrefabId);
				}
				const EncounterDefinition &definition =
					_dependencies.registries.encounters().get(p_request.encounter.encounterDefinitionId);
				BattleConstructionRequest construction{
					.descriptor =
						{.battleId = battleId,
						 .stableEncounterIdentity = p_request.stableEncounterInstanceId,
						 .encounterDefinitionId = p_request.encounter.encounterDefinitionId,
						 .encounterDisplayName = definition.displayNameKey,
						 .encounterKind = p_request.encounter.kind,
						 .allowsTaming = p_request.encounter.allowsTaming,
						 .repeatable = p_request.encounter.repeatable,
						 .boardSource = board.sourceDescriptor(),
						 .worldSeed = _dependencies.game.worldSeed,
						 .encounterOrdinal = p_request.encounterOrdinal,
						 .encounterResolutionSeed = p_request.encounterResolutionSeed,
						 .combatSeed = p_request.combatSeed,
						 .resolvedTier = p_request.encounter.resolvedTier,
						 .resolvedTeamId = p_request.encounter.teamId,
						 .returnWorldCell = p_request.worldCell},
					.participants = std::move(participants),
					.enemyPlacement = std::visit(
						[](const auto &policy) -> OpponentPlacementPolicy {
							return policy.opponentPlacement;
						},
						p_request.encounter.board)};
				BattleConstructionResult result =
					BattleSession::create(std::move(construction), _dependencies.registries, std::move(board));
				if (!result.succeeded())
				{
					throw std::runtime_error(result.error->diagnosticDetail);
				}
				// Stop at the support cell only after all fallible board/session preparation succeeded.
				_dependencies.playerActor.path.clear();
				_dependencies.playerActor.segment = 0;
				_dependencies.playerActor.segmentProgress = 0.0f;
				_dependencies.actorPath.placeAtCell(_dependencies.playerActor);
				_battle = std::make_unique<BattleModeRuntime>(
					std::move(p_request),
					std::move(result.session),
					_dependencies.fluidSimulator,
					fluidWasActive,
					_dependencies.dayTimeLogic,
					_dependencies.playerRenderer,
					_dependencies.explorationHoverRenderer,
					_dependencies.playerStreamer,
					_dependencies.battleStreamer,
					battleStreamerState.wasActive,
					battleStreamerState.origin,
					battleStreamerState.range,
					_dependencies.world,
					_dependencies.engine,
					_dependencies.voxelTexture,
					_dependencies.camera,
					_dependencies.maskTexture,
					_dependencies.battleHud,
					_dependencies.viewportSize,
					_dependencies.registries.gameRules(),
					handcraftedPrefab);
			} catch (...)
			{
				_dependencies.battleStreamer.setOriginPosition(battleStreamerState.origin);
				_dependencies.battleStreamer.setViewRange(battleStreamerState.range);
				if (battleStreamerState.wasActive)
				{
					_dependencies.battleStreamer.activate();
				}
				else
				{
					_dependencies.battleStreamer.deactivate();
				}
				if (fluidWasActive)
				{
					_dependencies.fluidSimulator.activate();
				}
				throw;
			}

			_activeGeneration = p_generation;
			_dependencies.game.control.mode = GameModeKind::Battle;
			_publishBatches();
			_dependencies.game.events.battleLifecycle.trigger(
				{.stage = BattleLifecycleStage::Entered,
				 .generation = _activeGeneration,
				 .battleId = _battle->session().battleId(),
				 .encounterDefinitionId = _battle->request().encounter.encounterDefinitionId});
		} catch (...)
		{
			if (_battle != nullptr)
			{
				_battle->shutdown();
				_battle.reset();
			}
			_dependencies.game.control.mode = GameModeKind::Exploration;
			_activeGeneration = {};
			_dependencies.game.events.battleLifecycle.trigger(
				{.stage = BattleLifecycleStage::EntryFailed,
				 .generation = p_generation,
				 .battleId = battleId,
				 .encounterDefinitionId = encounterId});
		}
	}

	void ModeManager::_exit(const BattleTerminalRecord &p_record)
	{
		if (_battle == nullptr)
		{
			return;
		}
		const BattleId battleId = _battle->session().battleId();
		const std::string encounterId = _battle->request().encounter.encounterDefinitionId;
		_dependencies.game.events.battleLifecycle.trigger(
			{.stage = BattleLifecycleStage::WillExit,
			 .generation = _activeGeneration,
			 .battleId = battleId,
			 .encounterDefinitionId = encounterId,
			 .outcome = p_record.outcome});
		_battle->shutdown();
		_battle.reset();
		_dependencies.game.control.mode = GameModeKind::Exploration;
		_dependencies.game.events.battleLifecycle.trigger(
			{.stage = BattleLifecycleStage::Exited,
			 .generation = _activeGeneration,
			 .battleId = battleId,
			 .encounterDefinitionId = encounterId,
			 .outcome = p_record.outcome});
		_activeGeneration = {};
		_resultPublished = false;
	}

	void ModeManager::processFrameBoundaryTransitions()
	{
		if (_stagedLiveEntry.has_value())
		{
			StagedLiveEntry staged = std::move(*_stagedLiveEntry);
			_stagedLiveEntry.reset();
			_enter(std::move(staged.request), staged.generation, staged.streamer);
			return;
		}
		if (const auto *request = std::get_if<BattleStartRequest>(&_pending))
		{
			BattleStartRequest value = std::move(*request);
			_pending = std::monostate{};
			try
			{
				_enter(std::move(value), _allocateGeneration());
			} catch (...)
			{
				// Allocation exhaustion is intentionally a refused entry, not a wrapped generation.
			}
			return;
		}
		if (const auto *record = std::get_if<BattleTerminalRecord>(&_pending))
		{
			BattleTerminalRecord value = std::move(*record);
			_pending = std::monostate{};
			_exit(value);
		}
	}

	void ModeManager::updateBattle(const spk::UpdateContext &p_tick)
	{
		if (_battle == nullptr || hasPendingTransition())
		{
			return;
		}
		_battle->update(p_tick);
		_publishBatches();
		if (_battle->session().phase() == BattlePhase::Terminal && !_resultPublished)
		{
			_resultPublished = true;
			const BattleTerminalRecord record = *_battle->session().terminalRecord();
			_dependencies.game.events.battleLifecycle.trigger(
				{.stage = BattleLifecycleStage::ResultReady,
				 .generation = _activeGeneration,
				 .battleId = record.battleId,
				 .encounterDefinitionId = _battle->request().encounter.encounterDefinitionId,
				 .outcome = record.outcome});
			(void)requestBattleExit(record);
		}
	}

	bool ModeManager::onMouseMoved(const spk::MouseMovedEvent &p_event)
	{
		return _battle != nullptr && !hasPendingTransition() && _battle->onMouseMoved(p_event);
	}

	bool ModeManager::onMousePressed(const spk::MouseButtonPressedEvent &p_event)
	{
		return _battle != nullptr && !hasPendingTransition() && _battle->onMousePressed(p_event);
	}

	bool ModeManager::onMouseReleased(const spk::MouseButtonReleasedEvent &p_event)
	{
		return _battle != nullptr && !hasPendingTransition() && _battle->onMouseReleased(p_event);
	}

	bool ModeManager::onMouseWheel(const spk::MouseWheelScrolledEvent &p_event)
	{
		return _battle != nullptr && !hasPendingTransition() && _battle->onMouseWheel(p_event);
	}

	bool ModeManager::onKeyPressed(const spk::KeyPressedEvent &p_event)
	{
		return _battle != nullptr && !hasPendingTransition() && _battle->onKeyPressed(p_event);
	}

	void ModeManager::onMouseLeave() noexcept
	{
		if (_battle != nullptr)
		{
			_battle->onMouseLeave();
		}
	}

	void ModeManager::shutdown() noexcept
	{
		_pending = std::monostate{};
		if (_stagedLiveEntry.has_value())
		{
			const BattleStreamerState &state = _stagedLiveEntry->streamer;
			_dependencies.battleStreamer.setOriginPosition(state.origin);
			_dependencies.battleStreamer.setViewRange(state.range);
			if (state.wasActive)
			{
				_dependencies.battleStreamer.activate();
			}
			else
			{
				_dependencies.battleStreamer.deactivate();
			}
			_stagedLiveEntry.reset();
		}
		if (_battle != nullptr)
		{
			_battle->shutdown();
			_battle.reset();
		}
		_dependencies.game.control.mode = GameModeKind::Exploration;
		_activeGeneration = {};
		_resultPublished = false;
	}
}
