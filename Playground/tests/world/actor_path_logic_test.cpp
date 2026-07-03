#include "components/actor.hpp"
#include "core/event_center.hpp"
#include "logics/actor_path_logic.hpp"
#include "structures/game_engine/spk_entity_3d.hpp"
#include "world/chunk_provider.hpp"
#include "world/voxel_world.hpp"
#include "world/world_navigation.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

namespace
{
	class FlatProvider final : public pg::IChunkProvider
	{
	private:
		std::int32_t _stone;

	public:
		explicit FlatProvider(std::int32_t p_stone) :
			_stone(p_stone)
		{
		}
		void fill(pg::Chunk &p_chunk) const override
		{
			for (int x = 0; x < 8; ++x)
			{
				p_chunk.grid().cell(x, 0, 0) = {_stone};
			}
			p_chunk.requestSynchronization();
		}
	};

	[[nodiscard]] const pg::VoxelRegistry &registry()
	{
		static const pg::VoxelRegistry result = [] {
			pg::VoxelRegistry loaded;
			loaded.load(std::filesystem::path(PG_RESOURCE_DIR) / "data" / "voxels");
			return loaded;
		}();
		return result;
	}
}

TEST(ActorPathLogic, EmitsReachedCellsInPathOrder)
{
	pg::VoxelWorld world(registry());
	FlatProvider provider(registry().numericId("stone-block"));
	(void)world.loadChunk({{0, 0, 0}}, provider);
	pg::WorldNavigation navigation(world, {{0, 0, 0}, {8, 4, 1}});
	pg::EventCenter events;
	std::vector<spk::Vector3Int> reached;
	auto contract = events.playerMoved.subscribe([&reached](spk::Vector3Int p_cell) {
		reached.push_back(p_cell);
	});
	pg::ActorPathLogic logic(events, navigation, world);
	spk::Entity3D entity;
	pg::Actor &actor = entity.addComponent<pg::Actor>();
	actor.cell = {0, 0, 0};
	actor.player = true;
	actor.speed = 1;

	logic.requestMove(actor, {3, 0, 0});
	logic.advance(actor, 3.1f);
	EXPECT_EQ(reached, (std::vector<spk::Vector3Int>{{1, 0, 0}, {2, 0, 0}, {3, 0, 0}}));
	EXPECT_EQ(actor.cell, spk::Vector3Int(3, 0, 0));
}

TEST(ActorPathLogic, RetargetsFromCurrentReachedCell)
{
	pg::VoxelWorld world(registry());
	FlatProvider provider(registry().numericId("stone-block"));
	(void)world.loadChunk({{0, 0, 0}}, provider);
	pg::WorldNavigation navigation(world, {{0, 0, 0}, {8, 4, 1}});
	pg::EventCenter events;
	pg::ActorPathLogic logic(events, navigation, world);
	spk::Entity3D entity;
	pg::Actor &actor = entity.addComponent<pg::Actor>();
	actor.cell = {0, 0, 0};
	actor.player = true;
	actor.speed = 1;

	logic.requestMove(actor, {6, 0, 0});
	logic.advance(actor, 1.2f);
	ASSERT_EQ(actor.cell, spk::Vector3Int(1, 0, 0));
	logic.requestMove(actor, {0, 0, 0});
	ASSERT_EQ(actor.path.front(), spk::Vector3Int(1, 0, 0));
	ASSERT_EQ(actor.path.back(), spk::Vector3Int(0, 0, 0));
	logic.advance(actor, 1.1f);
	EXPECT_EQ(actor.cell, spk::Vector3Int(0, 0, 0));
}
