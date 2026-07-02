# System — Audio

Minimal SFX + BGM service on **miniaudio** (vcpkg) — Sparkle has no audio support at all
(verified), and this stays a `pg::` concern until proven (D16).

Plan: step [35](../plan/steps/step-35-audio.md).

---

## Scope (deliberately small)

- Fire-and-forget **SFX** by id (`playSound("hit-blunt")`), optional volume/pitch variance.
- One **BGM** channel with crossfade (`playMusic("battle-theme")`, `stopMusic(fade)`).
- Master/SFX/BGM volume knobs (settings later; constants v1).
- No 3D spatialization v1 (top-down-ish camera distance makes it dispensable). Revisit
  after M1 content exists.

## Types (`Playground/srcs/audio/`)

```
pg::AudioService (owned at main scope, passed where needed — no singleton)
    init()/shutdown()                    // ma_engine lifecycle
    playSound(id, volume=1, pitchJitter=0)
    playMusic(id, fadeSeconds=1) / stopMusic(fadeSeconds)
    setVolume(channel, v)
pg::AudioRegistry   // scans resources/audio/{sfx,bgm}/*.wav|ogg → id = filename stem
                    // (a plain file scan, not a JSON domain — files are self-describing)
```

miniaudio usage: one `ma_engine`; SFX via `ma_engine_play_sound` (engine-managed
instances); BGM via one `ma_sound` per active track, fades with `ma_sound_set_fade_*`.
Decode formats: wav + ogg (miniaudio built-ins). All calls from the main thread; miniaudio's
own mixer thread does the work.

## Hook points (wired in step 35)

- Battle: enter → `battle-theme`, resolve → exploration theme; hit/heal/defeat SFX from
  battle events (a small `AudioBattleBinder` subscribing to the bus).
- Animation `playSound` steps ([animation.md](animation.md)) call the service.
- UI: button click SFX via a widget-level helper.

## Testing

Headless: registry scan, id resolution, service state machine (music replace/fade
bookkeeping) behind an `IAudioBackend` seam faked in tests (the real backend is a thin
miniaudio wrapper, untested). Sound output itself: hand-validated.
