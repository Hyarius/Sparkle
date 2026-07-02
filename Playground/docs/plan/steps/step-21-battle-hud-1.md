# Step 21 — Battle HUD I: team columns, active-unit panel, ability bar, turn strip

**Phase H · needs step 16 (can start before 17–20; parallel-safe with G)**

## Goal

The core battle HUD per the wireframes: both team columns, the bottom active-unit controls
with the 8-slot ability bar, and the left turn-order strip — all observable-bound.

## Reading

[ui-hud.md](../../03-systems/ui-hud.md) (layout truth incl. ASCII wireframes) ·
[howto/add-component-logic.md](../../howto/add-component-logic.md) (widget resize
convention: layout in `_onGeometryChange`) · existing spk widgets
(`spk_linear_layout`, `spk_text_label`, `spk_push_button`, `spk_slider_bar`).

## Files

`srcs/ui/`: `progress_bar_widget.hpp/.cpp` (labeled current/max bar — HP/AP/MP/turn-bar;
built on spk primitives), `creature_card_widget.hpp/.cpp` (compact + battle variants:
icon avatar cell, name, HP bar, turn bar; binds one BattleUnit's observables — store
Contracts, rebind on unit change), `team_column_widget.hpp/.cpp` (6 cards, side-tinted),
`ability_slot_widget.hpp/.cpp` (icon, AP/MP badge, hotkey label, disabled state from
validator, selected highlight), `ability_bar_widget.hpp/.cpp` (8 slots + page selector ±,
pages over the unit's ability list), `active_unit_panel.hpp/.cpp` (MP/HP/AP readouts +
ability bar + End Turn button), `turn_order_strip.hpp/.cpp` (per-unit mini turn-bar rows,
sorted by time-to-ready — recompute on turnBar notifications),
`battle_hud.hpp/.cpp` (assembles the tree; activated by BattleMode enter, deactivated on
exit — replaces nothing: M1 had no HUD).
`battle_input` — ability selection moves to bar clicks/hotkeys (bar drives the same
selection API); End Turn button → `EndTurnAction`.

## Work items

1. Binding discipline: every widget stores its Contracts; rebind on active-unit change
   (TurnStarted). Deactivate/reactivate cycles must not leak or double-fire (test below).
2. Disabled slots: grey when unaffordable or no valid target (validator queries — cheap,
   on turn start + after each resolution).
3. Layout per wireframe: columns at edges, controls bottom-center, strip left; world stays
   dominant (no full-width panels).

## Tests (`[test]`)

Headless widget logic: page math (9 abilities → 2 pages), slot enablement matrix, turn-strip
sort order vs computed time-to-ready, bind/unbind cycle safety (activate → deactivate →
activate; trigger observables; assert single notifications). Visual: everything else.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): full battle readable without the debug
overlay — team HP at a glance, turn order visible and honest (fast unit bubbles up), bar
shows costs and disables correctly, End Turn works, hover previews unchanged.
