#pragma once

#include "structures/game_engine/spk_component_logic.hpp"
#include "structures/voxel/spk_voxel_fluid_simulator.hpp"

namespace spk
{
	// Steps every processable VoxelFluidSimulator on the engine's update pass. Its default
	// priority sits between the chunk streamer (100, loads chunks) and the render logics
	// (0, bake meshes), so fluid edits made this update are baked in the same frame:
	// chunk streaming, fluid simulation, dirty chunk mesh construction, rendering.
	class VoxelFluidSimulationLogic : public spk::ComponentLogic<spk::VoxelFluidSimulator>
	{
	public:
		static constexpr spk::PriorizableTrait::Priority DefaultPriority = 50;
		[[nodiscard]] spk::RenderPhaseMask renderPhases() const noexcept override
		{
			return spk::RenderPhaseMask::None;
		}

		VoxelFluidSimulationLogic();

	protected:
		void _parseComponentForUpdate(const spk::UpdateContext &p_tick, spk::VoxelFluidSimulator &p_simulator) override;
	};
}
