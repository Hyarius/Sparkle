# Step 36 — Main menu, minimap, floating text, settings stub

**Phase L · needs steps 32, 34, 35**

## Goal

The deferred presentation shell: a main menu over a live world vista, the minimap (the
multi-camera payoff, D04), damage/heal floating text, and a minimal settings panel.

## Reading

[ui-hud.md §Main menu](../../03-systems/ui-hud.md) · wireframe `main_menu.png` ·
[rendering-cameras.md §4](../../03-systems/rendering-cameras.md) (second camera pass).

## Files

`srcs/core/main_menu_mode.hpp/.cpp` — third control-state (D03): world loads (last save's
seed or a showcase seed) with a slow orbiting vista camera; menu HUD on top: title
top-center, right-stacked New Game (seed prompt via `spk::PromptPanel`/text edit) /
Load (slot list) / Settings / Quit; entering gameplay = switch to ExplorationMode (no
reload — the vista world *is* the world when loading the same save). Boot now enters
MainMenuMode (step 32's auto-boot behavior moves behind `--skip-menu`).
`srcs/ui/minimap_widget.hpp/.cpp` — exploration HUD top-right: a second `Camera3D`
(top-down orthographic-ish high perspective) rendered via its own
CameraUpdate + viewport pass into the widget rect (`spk::ViewportRenderCommand` +
`UseFramebufferRenderCommand` if clipping demands — check both existing commands and pick
the simpler working path); player arrow overlay.
`srcs/ui/floating_text.hpp/.cpp` — world-anchored rising/fading labels on Damage (red),
Heal (green), feat completion (gold star) — spawned by a battle-event binder, projected via
the camera each frame.
`srcs/ui/settings_panel.hpp/.cpp` — volumes (audio service), invert camera drag, apply/
close; persisted in `saves/settings.json` (own tiny schema, version 1).

## Tests (`[test]`)

Mode-manager three-state transitions; settings round-trip; floating-text lifetime/
projection math (headless with a fixed camera); menu → load → exploration state chain.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): boot to the menu over a living world,
New Game/Load both land in exploration seamlessly, minimap tracks movement (and proves the
multi-camera architecture), numbers float on hits, settings persist across launches.
