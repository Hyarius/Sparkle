#include "structures/game_engine/spk_component_logic.hpp"

namespace spk
{
	IComponentLogic::IComponentLogic()
	{
		activate();
	}

	IComponentLogic::~IComponentLogic() = default;
}
