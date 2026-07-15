#pragma once

#include "battle/battle_lifecycle.hpp"
#include "battle/battle_outcome_rules.hpp"
#include "core/game_mode.hpp"

#include <memory>
#include <optional>
#include <variant>

namespace spk
{
	class GameEngine;
	class Texture;
	class TextureMeshRenderer3D;
	class VoxelChunkStreamer;
	class VoxelFluidSimulator;
}

namespace pg
{
	class Actor;
	class ActorPathLogic;
	class BattleSession;
	class Registries;
	class VoxelWorld;
	class WorldNavigation;
	struct GameContext;

	// Plain mode owner called only from GameSceneWidget's post-engine frame boundary.  It owns the
	// session and all temporary control suspension; GameContext deliberately never stores a session.
	class ModeManager
	{
	public:
		struct Dependencies
		{
			GameContext &game;
			const Registries &registries;
			VoxelWorld &world;
			WorldNavigation &worldNavigation;
			Actor &playerActor;
			ActorPathLogic &actorPath;
			spk::TextureMeshRenderer3D &playerRenderer;
			spk::TextureMeshRenderer3D &explorationHoverRenderer;
			spk::VoxelChunkStreamer &playerStreamer;
			spk::VoxelChunkStreamer &battleStreamer;
			spk::VoxelFluidSimulator &fluidSimulator;
			spk::GameEngine &engine;
			const spk::Texture &voxelTexture;
		};

	private:
		class BattleModeRuntime;
		struct BattleStreamerState
		{
			bool wasActive = false;
			spk::Vector3Int origin{};
			spk::Vector3Int range{};
		};
		struct StagedLiveEntry
		{
			BattleStartRequest request;
			BattleRuntimeGeneration generation;
			BattleStreamerState streamer;
		};
		Dependencies _dependencies;
		std::unique_ptr<BattleModeRuntime> _battle;
		std::variant<std::monostate, BattleStartRequest, BattleTerminalRecord> _pending;
		std::optional<StagedLiveEntry> _stagedLiveEntry;
		BattleRuntimeGeneration _nextGeneration{1};
		BattleRuntimeGeneration _activeGeneration;
		bool _resultPublished = false;

		[[nodiscard]] BattleRuntimeGeneration _allocateGeneration();
		void _publishBatches();
		void _enter(
			BattleStartRequest p_request,
			BattleRuntimeGeneration p_generation,
			std::optional<BattleStreamerState> p_stagedStreamer = std::nullopt);
		void _exit(const BattleTerminalRecord &p_record);

	public:
		explicit ModeManager(Dependencies p_dependencies);
		~ModeManager();
		ModeManager(const ModeManager &) = delete;
		ModeManager &operator=(const ModeManager &) = delete;

		[[nodiscard]] GameModeKind mode() const noexcept;
		[[nodiscard]] bool hasPendingTransition() const noexcept;
		[[nodiscard]] BattleSession *activeBattle() noexcept;
		[[nodiscard]] const BattleSession *activeBattle() const noexcept;
		[[nodiscard]] BattleId activeBattleId() const noexcept;

		bool requestBattle(BattleStartRequest p_request);
		bool requestBattleExit(BattleTerminalRecord p_record);
		void processFrameBoundaryTransitions();
		void updateBattle();
		void shutdown() noexcept;
	};
}
