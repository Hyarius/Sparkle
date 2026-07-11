# Make fluid simulation converge across streaming and edits

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Playground / Fluid simulation

## Problem

The fluid automaton is append-only and frontier-driven, but its frontier does not reliably survive streaming or world edits. Several state transitions can leave fluid permanently inconsistent:

- a chunk is scanned for sources only once, so a source added later is never discovered;
- an unloaded horizontal neighbor is skipped without retaining the current cell for retry;
- an occupied weaker fluid stage is never upgraded because only empty neighbors are written;
- settled flow never retracts after its source is removed or support/obstacles change;
- any nonempty non-fluid cell is treated as solid support, regardless of traversal semantics;
- discarded accumulated time slows simulation after long frames;
- unordered frontier/source iteration makes budget-limited evolution nondeterministic.

These behaviors mean the eventual state depends on load order, edit timing, and container order rather than only on the current world and source configuration.

## Evidence

- `playground/srcs/logics/fluid_simulation_logic.cpp:41-46` resets accumulated time to zero instead of retaining the interval remainder.
- `playground/srcs/logics/fluid_simulation_logic.cpp:62-117` scans only chunks absent from `_sourcesByChunk`.
- `playground/srcs/logics/fluid_simulation_logic.cpp:133-154` builds work from unordered source/frontier containers.
- `playground/srcs/logics/fluid_simulation_logic.cpp:194-207` treats every non-fluid nonempty voxel as solid support and retries only a specific unloaded-below case.
- `playground/srcs/logics/fluid_simulation_logic.cpp:230-240` silently skips unloaded, out-of-range, and already occupied horizontal neighbors.
- `playground/srcs/logics/fluid_simulation_logic.cpp:249-269` writes only empty cells and drops settled cells from the next frontier.
- `playground/srcs/logics/fluid_simulation_logic.hpp:42-45` stores the one-time source cache and unordered moving frontier.

## Failure scenario

Let a flow settle against the edge of an unloaded horizontal chunk. The boundary cell produces nothing and drops out of `_frontier`. Load the neighboring chunk later: no event reactivates that settled boundary, so fluid never crosses it.

Similarly, add a source to a chunk already represented in `_sourcesByChunk`; subsequent scans skip the chunk forever. Removing a source leaves all previously generated flow in place indefinitely.

## Proposed direction

Define a deterministic convergence model before changing individual conditions:

- notify the simulator of relevant cell and chunk load/unload changes;
- maintain dirty regions or dependencies so boundary cells retry when neighbors load;
- rescan or incrementally update source membership after edits;
- define stage precedence and allow stronger flows to replace weaker stages safely;
- define removal/retraction behavior and locally recompute affected flow when sources/support change;
- decide support through explicit voxel physical/traversal metadata rather than mere non-emptiness;
- retain tick remainder and process a bounded number of fixed steps;
- use a stable queue/order, or prove order independence for conflicting writes.

The result does not need to be a physically complete fluid solver, but its simplified rules must have explicit, reproducible lifecycle semantics.

## Acceptance criteria

- [ ] Fluid reaches a newly loaded horizontal neighbor without requiring the original source chunk to reload.
- [ ] Sources added to and removed from already scanned chunks update simulation state.
- [ ] A stronger incoming stage upgrades a weaker stage according to documented precedence.
- [ ] Removing a source or changing support produces the documented local retraction/reflow result.
- [ ] Passable non-fluid cells support or do not support fluid according to explicit metadata, not accidental non-emptiness.
- [ ] Equivalent worlds reach equivalent results regardless of chunk load order and unordered-container layout.
- [ ] Long frames retain elapsed-time remainder without unbounded catch-up.

## Tests

- [ ] Settle flow at an unloaded side boundary, load the neighbor, and verify propagation resumes.
- [ ] Add and remove a source in an already scanned chunk.
- [ ] Test stronger flow meeting a weaker stage from multiple directions.
- [ ] Add and remove support beneath settled fluid and verify recomputation.
- [ ] Test support behavior for passable vegetation and solid terrain.
- [ ] Run the same scenario with different chunk load and insertion orders and compare final grids.
- [ ] Test a long frame delta followed by normal deltas for fixed-step consistency.

## Dependencies / notes

Backlog item 005 covers the separate source-first work-budget starvation problem. Solve scheduler fairness and convergence with compatible queue semantics, but keep regression tests for the two failure classes distinct. If fluid traversal meaning changes, integrate its edits with the revision mechanism in ticket 024.
