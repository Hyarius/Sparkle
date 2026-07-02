# Step 28 — Encounter/Team + Ability/Status editors

**Phase I · needs steps 25, 23 (ai domain), 27 (node pickers)**

## Goal

The remaining authoring surfaces: encounter tables with tiered weighted teams, and
ability/status definitions with live rules-text preview — completing the tool suite for
content production.

## Reading

[tooling.md](../../03-systems/tooling.md) · wireframes `Encounter_Table_UI.png` +
`Team_Editor_WindowV2.png` (ASCII in Unity AGENTS.md) ·
[02-data-model.md §5/§4/§11](../../02-data-model.md).

## Files

`tools/pages/encounter_editor_page.hpp/.cpp` — left: table list; center: tier accordion
(the 10 tiers) → weighted team rows (name, weight, edit button); team editor sub-view:
6 slots, per-slot species IdPicker + ai IdPicker + **completedNodes picker** (checklist
over the species' board nodes — shows the derived form/stats summary live via
`applyProgress` on a scratch unit, D35); right: preview panel — a Viewport3D board preview
with the team auto-placed (reuses deployment rules).
`tools/pages/ability_editor_page.hpp/.cpp` — dual-domain page (tab-in-tab or list filter:
abilities | statuses): PropertyPanel for all fields (cost/range/AoE/profile spin-boxes +
enums, casterAnimation/targetAnimation pickers over animation sets when present, travelVfx
sub-panel, effect list with the **Effect factory** type picker; statuses: tags editor,
hookPoint enum, effect list); live **generated rules text** preview pane (step 15
`describe()`), icon atlas-cell picker.
`tools/pages/species_editor_page.hpp/.cpp` — species definitions: attributes grid,
defaultAbilities IdPickers, featBoard IdPicker, forms table (name/tier/model/animationSet/
avatar), tamingProfile condition list (**TamingCondition factory** type picker — the D33
split visible in tooling), model preview via Viewport3D.

## Tests (`[test]`)

Writer round-trips (tables incl. nulls/tiers; abilities incl. travelVfx/effects; statuses);
completedNodes checklist → derived summary correctness; describe-preview parity with the
HUD (same function — assert one shared symbol, not a copy).

## Definition of Done

`[tools]` (user validates): author a brand-new creature **entirely in tools** — species
(stats/forms/taming) + a new ability with two effects + a status + a feat board (step 27's
page) + an encounter team fielding it with completedNodes; save all; `[run]` — encounter
and fight it without touching a text editor. **Content production is now tool-complete.**
