# Make world ray hits respect actual voxel shape geometry

- **Status:** Complete — superseded by D43
- **Priority:** P1 — High, gameplay interaction correctness
- **Area:** Sparkle / Playground / voxel picking

## Resolution

The original ticket proposed narrow-phase intersection with oriented render polygons.
That direction was explicitly superseded by the user on 2026-07-11. Sparkle now exposes
`spk::VoxelRayCast`, intentionally defined as a low-cost unit-cell boundary DDA. It does
not inspect `VoxelShape` geometry.

The promoted implementation still resolves the robustness issues that remain relevant:

- finite and representable coordinates are validated before integer conversion;
- directions are normalized and zero directions are rejected;
- exact edge/corner ties advance every crossed axis simultaneously, avoiding arbitrary
  zero-volume intermediate cells;
- maximum distance and starting-inside behavior are documented and tested;
- selection policy is injected by the caller, so Playground keeps solid/passable
  gameplay semantics outside Sparkle.

Shape-accurate picking is not an outstanding requirement. If a future editor or gameplay
feature needs render-surface precision, it should introduce a separate narrow-phase API
rather than changing `VoxelRayCast`'s unit-cell contract.

## Verification

- [x] Axis-aligned boundary hit and maximum-distance tests.
- [x] Starting-inside behavior test.
- [x] Edge and corner tie tests with multi-axis entry normals.
- [x] NaN, infinity, extreme-coordinate, and zero-direction tests.
- [x] Predicate test proving Playground can ignore passable cells and select solid cells.
- [x] Sparkle non-visual suite and full Playground suite pass.
