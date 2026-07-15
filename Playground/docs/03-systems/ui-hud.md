# System — UI / HUD

Battle and exploration HUDs as Sparkle widgets, observable-bound, layout-true to the Unity
wireframes (`UI_Design/*.png`, ASCII versions in the Unity `AGENTS.md` — treat those as
layout truth; monochrome low-fi, not final art).

Plan: steps [21](../plan/steps/step-21-battle-hud-1.md),
[22](../plan/steps/step-22-battle-hud-2.md),
[24](../plan/steps/step-24-exploration-hud-interiors.md), 36 (menu).

---

## Principles

- Widgets **subscribe to observables and events; never poll** (`spk::ObservableValue`,
  `pg::ObservableResource`, EventCenter). Store the returned Contracts in the widget.
- HUD trees live as children of the window alongside `GameSceneWidget`; each mode
  activates/deactivates its tree on enter/exit (widgets are `spk::ActivableTrait`).
- Desktop-first, panel-based, screen-edge anchored; the world stays dominant. Use Sparkle
  layouts (`spk_linear_layout`, `spk_grid_layout`) and existing widgets (`spk::PushButton`,
  `spk::TextLabel`, `spk::Panel`, `spk::ProgressBar`-equivalent via `spk::SliderBar` or a
  small `pg::ProgressBarWidget`).
- M1 ships **no HUD** (debug overlay only, D05). Everything below lands in steps 21–24.

Step 13 provides the controller seam later buttons reuse: placement selection, movement, ability
selection, cancellation, deployment confirmation, and end turn all call
`BattleInteractionController`; HUD widgets must not create a second command route.

## Battle HUD (wireframe: `BattleHUD.png`)

```
+ Player team column (left)     + Enemy team column (right)
|  6× CreatureCardWidget        |  6× CreatureCardWidget
+ Turn-order strip (left edge, GDD: turn-bar progression display)
+ Bottom center: ActiveUnitPanel
   [MP cur/max] [HP cur/max] [AP cur/max]
   AbilityBarWidget: 8 slots + page selector (+/-), keyboard labels 1–8
   [End Turn]
```

- `pg::CreatureCardWidget` — compact (icon+name+HP), battle (adds turn bar), expanded hover
  (abilities row, stats, passive/status icons). Binds to one `BattleUnit`'s
  `BattleAttributes` + `statuses`.
- `pg::AbilityBarWidget` — 8 `AbilitySlotWidget` (icon, AP/MP cost badge, disabled state
  from `BattleActionValidator::canUseAbility`); click or key 1–8 → enters targeting;
  pages via selector when >8 abilities.
- `pg::ActiveUnitPanel` — binds to the active unit each `TurnStarted`.
- `pg::AbilityCardWidget` (hover detail, `Action_UI_Element.png`) — icon, name, costs,
  range/AoE/LoS metadata, **generated rules text**: each effect type renders a sentence from
  its fields ("Deals 5 magic damage (+100% Magic).", "Applies Burn (2 turns)."). One
  `describe(const Effect&)` function per effect type, colocated with the effect parsers.
- `pg::PassiveCardWidget` (`Passive_UI_Element.png`) — same generated-text approach.
- Battle result banner (step 12: debug text; step 22: proper panel) + **feat summary screen**
  (step 22): list per creature of nodes progressed/completed after `featProgressUpdated`.
- Creature info floating window (step 22, right-click a card): draggable `spk::InterfaceWindow`
  with stats/abilities/passives; **taming conditions section** visible only on wild enemies.
- Targeting flow (step 11): select ability → hovered cell drives `BoardOverlayState`
  (range/AoE/LoS tint) → click confirms (`AbilityAction`), Esc cancels; move mode: hover
  shows path + cost, click moves. Placement UI (step 22): click deployment cells to place
  team members in order, confirm button (M1 auto-places instead).

## Exploration HUD (wireframe: `Exploration_mode_screen.png`, step 24)

- Top-left: time/day-night widget (static placeholder text until a day cycle exists).
- Top-center: zone/biome name label (from active biome).
- Top-right: contextual icon buttons — v1: save, quit (menu/settings later).
- Left edge: vertical party strip, 6 compact `CreatureCardWidget`s bound to the team.
- Center stays world. Interaction prompt ("Enter", "Talk") appears near markers.

## Main menu (step 36, deferred — `main_menu.png`)

Title top-center, actions stacked right (New Game / Load / Quit), background = live vista of
the world (the engine renders behind the menu — answers the Unity open question with the
showcase option, cheap under Gamma since the world is always live).

## Testing

Widget logic that computes (rules-text generation, ability-slot enablement, page math) is
headless-tested; layout/visuals are hand-validated (project convention,
[howto/add-tests.md](../howto/add-tests.md)). Bind/unbind lifecycles: activate/deactivate a
HUD tree twice around a mode switch and assert no dangling-contract crashes (RAII contracts
make this near-automatic — the test guards regressions).
