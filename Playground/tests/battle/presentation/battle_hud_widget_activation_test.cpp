#include <gtest/gtest.h>

#include "widgets/battle/ability_slot_widget.hpp"
#include "widgets/battle/creature_card_widget.hpp"
#include "widgets/battle/resource_bar_widget.hpp"
#include "widgets/battle/status_effect_slot_widget.hpp"

TEST(BattleHudWidgets, CustomControlWrappersAreActivated)
{
	spk::Widget root("root");
	pg::CreatureCardWidget card("card", &root);
	pg::AbilitySlotWidget ability("ability", &root);
	pg::ResourceBarWidget resource("resource", &root);
	pg::StatusEffectSlotWidget effect("effect", &root);

	EXPECT_TRUE(card.isActivated());
	EXPECT_TRUE(ability.isActivated());
	EXPECT_TRUE(resource.isActivated());
	EXPECT_TRUE(effect.isActivated());
}

TEST(BattleHudWidgets, InspectionSlotsOnlyOpenDetailsForActualContent)
{
	spk::Widget root("root");
	pg::AbilitySlotWidget ability("ability", &root);
	pg::StatusEffectSlotWidget effect("effect", &root);
	int abilityDetails = 0;
	int effectDetails = 0;
	ability.setOnDetailsClick([&abilityDetails] {
		++abilityDetails;
	});
	effect.setOnDetailsClick([&effectDetails] {
		++effectDetails;
	});

	ability.render({.shortcut = 1});
	ability.invokeDetailsClick();
	EXPECT_EQ(abilityDetails, 0);
	ability.render({.shortcut = 1, .abilityId = "training-strike"});
	ability.invokeDetailsClick();
	EXPECT_EQ(abilityDetails, 1);

	effect.render({.id = "training-guarded", .displayName = "Guarded", .stacks = 2, .duration = "1 turn"});
	effect.invokeDetailsClick();
	EXPECT_EQ(effectDetails, 1);
}
