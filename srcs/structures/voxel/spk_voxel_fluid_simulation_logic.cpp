#include "structures/voxel/spk_voxel_fluid_simulation_logic.hpp"

namespace spk
{
	VoxelFluidSimulationLogic::VoxelFluidSimulationLogic()
	{
		setPriority(DefaultPriority);
	}

	void VoxelFluidSimulationLogic::_parseComponentForUpdate(
		const spk::UpdateContext &p_tick,
		spk::VoxelFluidSimulator &p_simulator)
	{
		if (p_simulator.hasLiveMap() == false)
		{
			return;
		}
		p_simulator.advance(static_cast<long long>(p_tick.deltaTime.milliseconds()));
	}
}
