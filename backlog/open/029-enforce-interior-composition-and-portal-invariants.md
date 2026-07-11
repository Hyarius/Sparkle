# Enforce interior composition and portal invariants

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Playground / Interior world generation

## Problem

Interior parsing and composition enforce only part of their implied contract:

- an entry room needs `entry` and `exit` anchors but is allowed to have no connector even when extra rooms are required;
- non-entry rooms pass validation with any anchor whose name starts with `connector:`, while composition recognizes only `+x`, `-x`, `+z`, and `-z`;
- composition stops after 64 attempts without checking that the selected minimum/target room count was reached;
- room slot containment checks only X and Z, not Y;
- generated portal sources and targets are not validated for uniqueness, solidity, headroom, or graph membership;
- duplicate portal sources are silently discarded when runtime uses `emplace`.

The plan can therefore report a composed interior that is smaller than requested, disconnected, outside its reserved volume, or unreachable.

## Evidence

- `playground/srcs/world/interior_parser.cpp:27-39` applies different anchor checks to entry and non-entry rooms and accepts any `connector:` prefix.
- `playground/srcs/world/interior_parser.cpp:71-79` validates room-count bounds but not whether the entry topology can satisfy them.
- `playground/srcs/world/generator/world_plan_generator.cpp:2467-2492` recognizes only four exact connector suffixes.
- `playground/srcs/world/generator/world_plan_generator.cpp:2574-2593` emits rooms and connector candidates without validating final navigation semantics.
- `playground/srcs/world/generator/world_plan_generator.cpp:2600-2604` computes a target but caps placement at 64 attempts.
- `playground/srcs/world/generator/world_plan_generator.cpp:2631-2641` checks slot containment only in X/Z.
- `playground/srcs/world/generator/world_plan_generator.cpp:2661-2664` unconditionally emits two portals and counts the interior as composed.
- `playground/srcs/game_scene_widget.cpp:170-174` installs portal sources with `emplace` and does not inspect insertion failure.

## Failure scenario

Define an interior with `minRooms = 2` and an entry prefab containing only `entry` and `exit` anchors. Parsing succeeds. Composition places the entry, has no open connector, places no extra rooms, still emits portals, and increments the composed-interior statistic.

Another definition can use `connector:north`; it passes parser validation but is ignored by `connectorDirection()`, producing the same silent under-composition.

## Proposed direction

- Share one exact connector-name parser between asset validation and composition.
- Require a usable entry connector whenever the configured minimum requires additional rooms.
- Define whether `minRooms` counts extra rooms or total rooms and use that meaning consistently in data, stats, and reports.
- Make failure to reach the chosen target explicit: retry with bounded backtracking, accept only a documented reduced-size policy, or reject the generated plan.
- Validate room boxes against the full X/Y/Z slot and all relevant claims.
- Validate portal source uniqueness and source/target standability, headroom, and realized graph membership before publishing the plan.
- Treat failed runtime insertion as an error rather than silently dropping a transition.

## Acceptance criteria

- [ ] Parser and composer accept the same exact connector grammar.
- [ ] An entry incapable of satisfying the minimum room topology is rejected with file/path context.
- [ ] Successful composition meets the documented room-count guarantee.
- [ ] Every room remains within the reserved slot on X, Y, and Z and does not overlap another room illegally.
- [ ] Every portal source is unique and every source/target resolves to a valid traversable cell with required headroom.
- [ ] Statistics and warnings accurately distinguish composed, reduced, and failed interiors.

## Tests

- [ ] Reject an entry with no connector when additional rooms are required.
- [ ] Reject unsupported connector names such as `connector:north`.
- [ ] Exercise a cramped slot/placement sequence that cannot meet the minimum.
- [ ] Exercise a room that fits X/Z but exceeds Y bounds.
- [ ] Reject duplicate portal sources and blocked or headroom-less targets.
- [ ] Verify valid seeded interiors remain deterministic and reachable end to end.

## Dependencies / notes

Portal validity may need a plan-time geometric check followed by a realized-world/navigation assertion. Coordinate any shared transition mechanism with ticket 028. Clarify room-count semantics before changing existing data files.
