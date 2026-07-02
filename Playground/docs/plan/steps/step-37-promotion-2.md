# Step 37 — Promotion II: Viewport3D, GraphCanvas, PropertyPanel → `spk::`

**Phase M · needs steps 25–28 proven in daily use · ⚠ user sign-off required (D06/D18)**

## Goal

Lift the tool-framework widgets into Sparkle — the engine's payoff from the tooling track.

## Reading

[howto/promote-to-spk.md](../../howto/promote-to-spk.md) (checklist — follow verbatim) ·
[tooling.md §shared building blocks](../../03-systems/tooling.md).

## What moves

| pg::tools:: | spk:: target |
|---|---|
| `Viewport3D` | `includes/structures/widget/spk_viewport_3d.hpp/.cpp` (depends on the step-13 promoted 3D layer) |
| `GraphCanvas` | `widget/spk_graph_canvas.hpp/.cpp` (generic node/edge canvas — feat-board-specific rendering stays in the tools page) |
| `PropertyPanel` (+ `IdPicker` generalized to a `spk::SearchableDropdown`) | `widget/spk_property_panel.hpp/.cpp`, `widget/spk_searchable_dropdown.hpp/.cpp` — the polymorphic type picker generalizes to a name-list + callback (no `pg::PolymorphicFactory` dependency in `spk::`) |

Stays in `pg::`: `AtlasCellPicker` (game-atlas-shaped), `JsonWriter` (nlohmann dependency
stays out of `spk::`, D02), all pages.

## Work items

1. Proposal + sign-off (API changes made while lifting — especially decoupling
   PropertyPanel from the factory type).
2. Move, restyle, re-point tool pages; delete originals.
3. Library tests: `tests/TUs/srcs/structures/widget/` — canvas hit-test/zoom-pan math,
   property-panel row plumbing, dropdown filtering (widget logic is testable headless;
   rendering is not — follow the existing widget tests' conventions if present, else test
   the math classes only).
4. `[spk-test]` + clang-tidy check clean; UIShowcase gains a demo page for the three
   widgets (the library's own showcase habit).

## Definition of Done

`[spk-test]`/`[test]`/`[tools]` all green; tools behave identically; UIShowcase demos the
promoted widgets; decision-log entry. **The plan is complete — remaining work is content,
balancing, and new decisions logged as they arise.**
