#include "core/event_center.hpp"

#include <gtest/gtest.h>

TEST(EventCenter, DeliversTypedEventPayload)
{
	pg::EventCenter events;
	spk::Vector3Int received;
	int calls = 0;
	auto contract = events.playerMoved.subscribe([&](spk::Vector3Int p_position) {
		received = p_position;
		++calls;
	});

	events.playerMoved.trigger({2, 3, 4});

	EXPECT_EQ(calls, 1);
	EXPECT_EQ(received, spk::Vector3Int(2, 3, 4));
}

TEST(EventCenter, DroppingContractResignsSubscription)
{
	pg::EventCenter events;
	int calls = 0;
	{
		auto contract = events.playerMoved.subscribe([&calls](spk::Vector3Int) {
			++calls;
		});
		events.playerMoved.trigger({1, 0, 0});
	}

	events.playerMoved.trigger({2, 0, 0});
	EXPECT_EQ(calls, 1);
}

TEST(EventCenter, ExplicitResignStopsSubscription)
{
	pg::EventCenter events;
	int calls = 0;
	auto contract = events.playerMoved.subscribe([&calls](spk::Vector3Int) {
		++calls;
	});

	contract.resign();
	events.playerMoved.trigger({1, 0, 0});
	EXPECT_EQ(calls, 0);
}
