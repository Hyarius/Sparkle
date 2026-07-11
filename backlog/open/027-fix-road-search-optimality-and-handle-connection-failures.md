# Fix road search optimality and handle connection failures

- **Status:** Ready
- **Priority:** P1 — High
- Area: Playground / World-generation roads

## Problem

The road A* heuristic assumes cardinal movement costs at least 1 and diagonal movement costs at least square-root-of-two. Reusing an existing road multiplies a step cost by 0.25, so the heuristic can overestimate the cheapest remaining route. Because search stops as soon as the goal is popped, the result is not guaranteed to have the lowest configured cost.

Road construction also returns `false` when no path exists, but all settlement, point-of-interest, and gateway callers ignore that result. Required links can silently be absent, leaving inferior or disconnected networks without an actionable generation error.

## Evidence

- `playground/srcs/world/generator/world_plan_generator.cpp:1296-1332` computes road step costs and applies the `0.25` reuse multiplier.
- `playground/srcs/world/generator/world_plan_generator.cpp:1337-1344` uses an octile heuristic based on unit minimum costs.
- `playground/srcs/world/generator/world_plan_generator.cpp:1358-1365` exits when the goal is first popped.
- `playground/srcs/world/generator/world_plan_generator.cpp:1471-1478` exposes success from `connectRoad()`.
- `playground/srcs/world/generator/world_plan_generator.cpp:1566-1569` ignores settlement connection failures.
- `playground/srcs/world/generator/world_plan_generator.cpp:1583-1591` ignores selected POI connection failures.
- `playground/srcs/world/generator/world_plan_generator.cpp:1596-1609` ignores both sides of primary gateway connection failures.

## Failure scenario

Construct a plan where a longer geometric detour mostly follows existing roads and is cheaper than a shorter all-new route. The current heuristic can favor and finalize the more expensive route. In a second case, make a required gateway unreachable under water, stair-height, or square-prevention constraints; `connectRoad()` returns false and generation continues without connecting the zones.

## Proposed direction

- Make the heuristic admissible and preferably consistent for the actual minimum edge costs, or use Dijkstra when dynamic road reuse costs make a useful proof difficult.
- Verify diagonal/elbow costs against the final orthogonal cells that are materialized.
- Require callers to handle `connectRoad()` failure explicitly: retry with a documented fallback, record a structured warning, or reject/regenerate the plan depending on connection type.
- Validate final settlement and primary-gateway connectivity after road-square cleanup.
- Keep seeded generation deterministic.

## Acceptance criteria

- [ ] The search returns a minimum-cost route under the configured `stepCost`, as checked against a reference shortest-path implementation.
- [ ] Existing-road reuse remains preferred without invalidating heuristic assumptions.
- [ ] Every required settlement and primary gateway connection either succeeds or produces an explicit, surfaced failure outcome.
- [ ] Post-processing cannot silently disconnect a network that was reported as connected.
- [ ] World reports distinguish hard connection failures from optional POI omissions.

## Tests

- [ ] Add a crafted map where the cheapest route is a longer existing-road detour.
- [ ] Compare A* route cost with Dijkstra over many small randomized grids.
- [ ] Add unreachable settlement and gateway cases and assert the selected failure policy.
- [ ] Verify diagonal elbow materialization cost and connectivity.
- [ ] Run deterministic multi-seed road/component checks.

## Dependencies / notes

The desired policy may differ by link type: settlement and primary gateway links appear mandatory, while POI roads may be optional. Record that policy rather than treating every failure identically. Boat links in ticket 028 should not be used to conceal missing runtime road connectivity.
