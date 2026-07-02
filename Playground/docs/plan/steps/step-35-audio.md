# Step 35 — Audio: miniaudio service + battle/UI/step hooks

**Phase L · needs step 01 (vcpkg) · parallel-safe with 34 (playSound step wiring merges
with it)**

## Goal

Sound (D16): SFX + crossfading BGM on miniaudio, hooked to battle events, UI, and
animation `playSound` steps.

## Reading

[audio.md](../../03-systems/audio.md) (the spec) · miniaudio's single-header docs
(`ma_engine`, `ma_engine_play_sound`, `ma_sound`, fades).

## Files

- **Modify** `vcpkg.json` — add `"miniaudio"`; reconfigure.
- `srcs/audio/`: `i_audio_backend.hpp` (seam: playSound/startMusic/stopMusic/setVolume —
  faked in tests), `miniaudio_backend.hpp/.cpp` (one `ma_engine`; SFX fire-and-forget with
  volume/pitch jitter; BGM = `ma_sound` per track with fade in/out), `audio_service.hpp/
  .cpp` (id resolution, channel volumes, music replace bookkeeping), `audio_registry.hpp/
  .cpp` (scan `resources/audio/{sfx,bgm}/*.{wav,ogg}`, id = stem),
  `audio_binder.hpp/.cpp` (subscriptions: battleStarted → battle theme, battleResolved →
  exploration theme, Damage/Heal/ShieldBroken/UnitDefeated → SFX, UI click helper;
  animation `playSound` steps call the service — coordinate with step 34's step factory).
- `main` owns the service; passed by reference (no singleton).
- Data: placeholder audio files (any free/self-made wav/ogg: 2 BGM loops, ~6 SFX) under
  `resources/audio/`.

## Tests (`[test]`)

Registry scan/id resolution; service state machine over the fake backend (music replace →
stop+fade then start; volume routing; unknown id → warn, no crash); binder event→sound
mapping table.

## Definition of Done

`[build]`/`[test]` green; `[run]` (user validates): exploration theme plays, battle entry
crossfades to battle theme and back, hits/heals/defeats are audible, buttons click.
No stutter on encounter transitions.
