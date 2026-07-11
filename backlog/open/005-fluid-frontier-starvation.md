# Make fluid simulation scheduling fair under its per-tick budget

- **Status:** Ready
- **Priority:** P1 — High, simulation correctness
- **Area:** Playground / fluid simulation / scheduling

## Problem

Each fluid step rebuilds its work list with every in-range persistent source first and the existing flow frontier second. Processing stops at `_maxCellsPerTick`; deferred entries are placed in the next frontier. On the following step, however, the complete source list is prepended again before those deferred frontier entries.

When the number of in-range sources meets or exceeds the budget, frontier cells can be deferred forever. The default horizontal streaming window is 5×5 chunks, each 16×16 columns, or 6,400 columns. A sufficiently broad ocean can therefore exceed the 6,000-cell budget with source cells alone, leaving no opportunity for any flow front to advance.

Iteration through unordered source containers also means which subset receives service is not deterministic across container order changes.

## Evidence

- Persistent sources are appended before `_frontier`: [`playground/srcs/logics/fluid_simulation_logic.cpp:133`](../../playground/srcs/logics/fluid_simulation_logic.cpp#L133).
- Processing stops at `_maxCellsPerTick` and places the remainder in `next`: [`playground/srcs/logics/fluid_simulation_logic.cpp:154`](../../playground/srcs/logics/fluid_simulation_logic.cpp#L154).
- The budget defaults to 6,000 cells: [`playground/srcs/logics/fluid_simulation_logic.hpp:38`](../../playground/srcs/logics/fluid_simulation_logic.hpp#L38).
- Voxel chunks are 16×16×16: [`includes/structures/voxel/spk_voxel_chunk.hpp:15`](../../includes/structures/voxel/spk_voxel_chunk.hpp#L15).
- Playground's horizontal stream range is two chunks in each direction: [`playground/srcs/game_scene_widget.cpp:41`](../../playground/srcs/game_scene_widget.cpp#L41).

## Failure scenario

1. Load a 5×5 horizontal chunk window containing at least 6,000 in-range persistent fluid sources.
2. Ensure at least one source has produced or should produce a frontier cell.
3. Each tick spends its entire budget on the source prefix.
4. The frontier is copied into deferred work but remains after all sources on every subsequent tick.
5. Flow away from sources never progresses, despite the simulation continuing to tick.

## Proposed direction

Use a fair, persistent scheduling policy. Possible designs include separate source and frontier budgets, rotating cursors over source chunks, a unified queue with bounded re-enqueueing, or weighted round-robin service. Preserve a hard frame-time budget while guaranteeing that a continuously eligible frontier item eventually receives service.

For reproducibility, define ordering explicitly where competing writes can affect results rather than relying on `unordered_map`/`unordered_set` traversal order.

## Acceptance criteria

- [ ] Frontier work makes bounded progress even when source count exceeds the per-tick budget.
- [ ] Persistent sources continue to be revisited without monopolizing all work.
- [ ] The hard per-tick processing cap remains effective.
- [ ] Deferred work is not reinserted permanently behind a freshly rebuilt unbounded prefix.
- [ ] Outcomes are deterministic for a fixed world, seed, configuration, and tick sequence, or nondeterminism is explicitly accepted and documented.
- [ ] Scheduling remains efficient when most sources are out of range.

## Tests

- [ ] Create more sources than `_maxCellsPerTick` and prove a frontier item advances within a bounded number of steps.
- [ ] Test source counts just below, equal to, and above the budget.
- [ ] Test multiple loaded chunks and repeated range changes.
- [ ] Verify the number of processed cells never exceeds the configured cap.
- [ ] Repeat the same setup and compare resulting fluid grids for determinism.

## Dependencies / notes

This ticket addresses starvation specifically. Retry behavior at unloaded horizontal boundaries, source discovery after edits, fluid retraction, and support semantics are separate fluid-convergence concerns.
