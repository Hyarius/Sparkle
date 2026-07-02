# Step 22 — Battle HUD II: hover cards, placement UI, result + feat summary, info window

**Phase H · needs steps 18, 21 (feat summary needs 18)**

## Goal

Battle UX completeness: expanded hover cards with generated rules text, manual placement,
a real result screen with the feat summary, and the creature info window (with taming
conditions on wild enemies).

## Reading

[ui-hud.md](../../03-systems/ui-hud.md) (card states, ability card, placement note) ·
`spk_interface_window.hpp` (floating window widget).

## Files

`srcs/ui/`: `creature_card_widget` grows the expanded hover state (abilities row, stat
grid, passive/status icon row with `describe()` tooltips), `ability_card_widget.hpp/.cpp`
(hover detail: icon/name/costs/range/AoE/LoS metadata + **generated rules text** from
step 15's `describe()`), `passive_card_widget.hpp/.cpp`,
`placement_ui.hpp/.cpp` — PlacementPhase goes interactive for the player side: deployment
masks shown (step 11), click a team card then a zone cell to place/swap, Confirm button
enables when all placed (auto-place remains for enemies and as a "Auto" button);
PlacementPhase's completion signal now waits on Confirm,
`battle_result_screen.hpp/.cpp` — replaces the banner: outcome title, per-creature feat
summary rows (from `featProgressUpdated` payloads + node names/uuid diff), recruited
creatures (taming), Continue button,
`creature_info_window.hpp/.cpp` — right-click a card → draggable/closable
`spk::InterfaceWindow`: stats/abilities/passives; **taming conditions section** on wild
enemies (condition list + live progress bars from TamingProgress — visible once any
progress > 0, else "???" rows).
`battle_mode` — result screen gates `battleResolved` → exploration hand-back.

## Tests (`[test]`)

Placement state machine headless (select/place/swap/confirm-enable); rules-text goldens for
composed abilities (multi-effect sentences); feat-summary assembly from payload fixtures;
info-window content selection (wild vs owned vs trainer enemy).

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): place your team by hand, fight, see a
result screen listing exactly the feats that progressed, recruit line when tamed, info
window shows taming progress live during the fight.
