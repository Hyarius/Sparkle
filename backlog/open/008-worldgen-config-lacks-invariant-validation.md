# Validate world-generation configuration before allocating or generating

- **Status:** In Progress
- **Priority:** P1 — High, robustness and undefined behavior
- **Area:** Playground / world generation / configuration

## Problem

`WorldGenConfig` exposes many numeric fields with cross-field invariants, but generation validates only that at least one biome exists. `PlanGrid` accepts any signed size, casts it to `size_t`, multiplies it, and exposes an `at()` method with no bounds checking.

Invalid configurations can therefore be silently accepted, request pathological allocations, divide by zero, violate standard-library preconditions, narrow out-of-range values into compact plan grids, or overflow signed coordinate arithmetic. Examples include non-positive `size`, non-positive `blocksPerCell`, negative `zoneCount`, negative `maxHeightLevel`, inverted lake/stair limits, and products that do not fit the coordinate types.

The CLI currently accepts `--size -1`; a diagnostic run produced an empty/nonsensical plan and failed only later because no stairways were generated, rather than reporting the invalid configuration at its boundary.

## Evidence

- `PlanGrid` converts and squares `p_size` without validation: [`playground/srcs/world/generator/world_plan.hpp:27`](../../playground/srcs/world/generator/world_plan.hpp#L27).
- `PlanGrid::at()` performs unchecked mixed signed/unsigned indexing: [`playground/srcs/world/generator/world_plan.hpp:41`](../../playground/srcs/world/generator/world_plan.hpp#L41).
- The public configuration includes interdependent size, zone, height, count, scale, and realization fields: [`playground/srcs/world/generator/world_plan.hpp:169`](../../playground/srcs/world/generator/world_plan.hpp#L169).
- Generator construction validates only the biome list before allocating plan grids: [`playground/srcs/world/generator/world_plan_generator.cpp:353`](../../playground/srcs/world/generator/world_plan_generator.cpp#L353).
- A negative `zoneCount` is passed to `std::vector<int>` through signed-to-unsigned conversion: [`playground/srcs/world/generator/world_plan_generator.cpp:401`](../../playground/srcs/world/generator/world_plan_generator.cpp#L401).
- World-coordinate helpers use unchecked multiplication, division, negation, and addition in `noexcept` functions: [`playground/srcs/world/generator/world_plan.hpp:297`](../../playground/srcs/world/generator/world_plan.hpp#L297).

## Failure scenario

1. Pass `WorldGenConfig{.size = -1}` or use the CLI equivalent.
2. `PlanGrid` converts the negative dimension to `size_t`; loop behavior and allocation size no longer describe a valid square plan.
3. Generation continues far enough to return empty or misleading output instead of identifying the bad field.

Other inputs, such as `blocksPerCell == 0`, reach coordinate conversion division; `maxHeightLevel < 0` can make a `std::clamp` lower bound exceed its upper bound; extreme products can overflow before world coordinates are returned.

## Proposed direction

Add one authoritative validation step at the public generation boundary, before any allocation or arithmetic derived from the configuration. Validate individual ranges, finiteness of floating values, required positivity, min/max relationships, and checked products/sums. Return actionable errors naming the offending field and constraint.

`PlanGrid` should independently protect its own general-purpose invariants rather than assuming every caller validated them. Decide deliberately whether `at()` should become checked or be renamed/documented as unchecked with a separate checked accessor.

## Acceptance criteria

- [ ] Invalid configuration is rejected before plan-grid allocation and generation stages begin.
- [ ] Errors identify the field and violated constraint.
- [ ] `size`, `zoneCount`, `blocksPerCell`, and `blocksPerLevel` have explicit valid ranges.
- [ ] Count, scale, fraction, radius, and min/max pairs have documented finite ranges and relationships.
- [ ] All products and coordinate bounds needed by `PlanGrid` and `WorldPlan` are proven representable or computed with checked arithmetic.
- [ ] Compact storage limits (`int8_t` heights and `int16_t` zones) are validated or widened.
- [ ] `PlanGrid` cannot be constructed with a dimension that makes its storage/index arithmetic invalid.
- [ ] CLI paths report the same validation error instead of silently producing an empty plan.

## Tests

- [ ] Add table-driven validation tests for zero, negative, and just-outside-limit values for every integer field.
- [ ] Test NaN/infinity and invalid ranges for every floating field.
- [ ] Test product-overflow boundaries for plan area and world extent.
- [ ] Test `zoneCount` and `maxHeightLevel` against their storage limits.
- [ ] Test inverted lake and stair limits.
- [ ] Keep deterministic generation coverage for the default configuration and representative valid boundary configurations.
- [ ] Add CLI regression coverage for `--size -1`, `--size 0`, and an excessive size.

## Dependencies / notes

Choose practical upper bounds based on expected memory/time budgets, not only integer representability. This ticket should establish validation before performance-oriented incremental generation work relies on these values.
