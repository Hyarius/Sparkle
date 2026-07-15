#pragma once

#include <cstdint>

namespace pg
{
	class BattleContext;

	// The deterministic material-state digest (section 15), for tests and later AI no-progress
	// guards. It folds every fact that can change the result of the battle - board source kind and
	// handcrafted definition id, the immutable board topology (extent, bounded voxels, navigation
	// nodes/edges with costs, deployment zones, border cells), the live terrain provenance
	// (world anchor and the per-required-chunk content revisions) or its explicit handcrafted
	// absence, plus phase, outcome, elapsed, active/turn, every unit's resources and occupancy, and
	// the RNG draw count.
	//
	// It deliberately excludes event/action/batch counters, log length, presentation state,
	// diagnostic traces and memory addresses, and never hashes presentationOrigin as an independent
	// transform or the stamp's borrowed world pointer/lifetime token or any current live revision
	// read after construction. Two equivalent reconstructed boards therefore compare equal by value.
	[[nodiscard]] std::uint64_t gameplayProgressDigest(const BattleContext &p_context);

	// The replay/orchestration digest builds on the material digest and additionally includes the
	// generic command counter.  AI-owned activation guard scratch is mixed by BattleAIDriver,
	// where that scratch is actually owned; it intentionally never enters a BattleSnapshot.
	[[nodiscard]] std::uint64_t authoritativeBattleStateDigest(const BattleContext &p_context);
}
