#include "creatures/apply_progress.hpp"

#include "creatures/creature_species.hpp"
#include "creatures/creature_unit.hpp"
#include "feats/feat_board.hpp"
#include "feats/feat_progress.hpp"

#include <algorithm>
#include <stdexcept>

namespace pg
{
	void applyProgress(CreatureUnit &p_unit)
	{
		if (p_unit.species == nullptr)
		{
			throw std::invalid_argument("applyProgress requires a creature species");
		}
		const int previousHealth = p_unit.currentHealth;
		p_unit.attributes = p_unit.species->attributes;
		if (previousHealth >= 0)
		{
			p_unit.currentHealth = std::min(previousHealth, p_unit.attributes.health);
		}
		p_unit.abilities = p_unit.species->defaultAbilities;
		p_unit.permanentPassives.clear();
		p_unit.currentFormId = p_unit.species->defaultFormId;
		if (p_unit.species->featBoard == nullptr)
		{
			return;
		}
		for (const FeatNode &node : p_unit.species->featBoard->nodes)
		{
			const FeatNodeProgress *progress = p_unit.featBoardProgress.findProgress(node.uuid);
			if (progress == nullptr) continue;
			for (int completion = 0; completion < progress->completionCount; ++completion)
				for (const auto &reward : node.rewards) reward->apply(p_unit);
		}
	}
}
