#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "world/chunk.hpp"

namespace pg
{
	class ChunkSynchronizationLogic : public spk::ComponentLogic<Chunk>
	{
	public:
		void process(Chunk &p_chunk) const
		{
			if (p_chunk.needsSynchronization())
			{
				p_chunk.synchronize();
			}
		}

	protected:
		void _parseComponentForUpdate(const spk::UpdateTick &, Chunk &p_chunk) override
		{
			process(p_chunk);
		}
	};
}
