# Step 12 — **Milestone 1**: end-to-end wiring, unit views, move tween, result banner

**Phase D · critical path · needs step 11**

## Goal

Close the loop (D05): bush → battle → resolve → free-roam, with visible units that glide
between cells, a win/lose banner, and clean state restoration. **Milestone 1 done.**

## Reading

[22-seq-encounter-to-battle.puml](../../diagrams/22-seq-encounter-to-battle.puml) ·
[battle.md §7](../../03-systems/battle.md) · [world.md §Actors](../../03-systems/world.md).

## Files

`srcs/components/battle_unit_view.hpp` + `srcs/logics/battle_unit_view_logic.hpp/.cpp` —
one view entity per placed `BattleUnit` (placeholder cube, tinted per side, D26): spawns on
`UnitRegistered`-equivalent (placement), positions at cell walk height + 0.5, **tweens**
through the path cells on move resolution (reuse the actor segment interpolation), sinks/
hides on defeat, despawns at battle end.
`srcs/ui/battle_banner.hpp/.cpp` — minimal centered `spk::TextLabel` panel: "Victory!" /
"Defeat…" + "click to continue"; click (or 2 s) → confirms battle end.
`srcs/core/battle_mode.*` + `mode_manager.*` — full exit path: `battleResolved` → banner →
overlay drop, camera blend back, exploration input/emitter unfreeze, encounter emitter
same-cell state reset (no instant re-trigger on the same bush), remove
`runHeadlessDemo` hook from step 10.
Loss flow (M1 minimal): player position unchanged, log line "(respawn comes with saves,
step 32)".

## Work items

1. Move resolution is still instant in the backend; the view tween is presentation-only —
   input is locked while the tween plays (a simple `viewBusy` flag the input honors) so the
   next action reads a settled board.
2. Turn banner mini-feedback: overlay the active unit's cell with the `hovered` mask at
   turn start (1 s) — enough turn feedback without HUD (D05).
3. End-to-end sweep: three consecutive encounters in one session (win, lose, win) — state
   must be pristine each time (unit views, overlay, camera, input, emitter).

## Tests (`[test]`)

Mode round-trip state machine (enter/exit battle twice — subscriptions, input activation,
overlay lifetime asserted via counters). View-spawn bookkeeping headless (view registry
sizes across place/defeat/end). Everything else visual.

## Definition of Done — **Milestone 1 checklist (user validates end-to-end)**

- `[build]`/`[test]` green; boot straight into the testground (no menu).
- Click-to-move exploration; slopes/stairs/wall respected; hover highlight.
- Bush → battle overlay in place, no scene cut, camera blend both ways.
- Auto-placement; turn-bar order honored (fast unit acts more often — set enemy stamina 6
  vs player 3 in data to make it visible); Move + Tackle + End Turn via mouse/keys; masks
  drape terrain; LoS blocked behind the wall.
- Enemy moves toward and attacks (stand-in AI); defeat removes units; victory/defeat banner;
  return to free-roam; repeatable across ≥3 encounters without restart.
- Commit tagged `milestone-1` after user sign-off.
