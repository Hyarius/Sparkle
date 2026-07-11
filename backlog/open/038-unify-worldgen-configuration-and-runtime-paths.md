# Ticket 038 — Unify world-generation configuration and runtime resource/output paths

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Playground / configuration / packaging

## Problem

The repository contains a tracked `worldgen/default.json`, but runtime never reads it and instead constructs C++ defaults. The accepted `--size` option applies only to map/stair modes and is silently ignored by the interactive game. Startup also rewrites the tracked source-tree `world_map.png`, while resources are located through a compiled absolute checkout path. The result is multiple misleading sources of truth and a non-relocatable executable that expects a writable source checkout.

## Evidence

- `playground/resources/data/worldgen/default.json:1-37` contains an unused configuration schema.
- `playground/srcs/core/registries.cpp:55-58` loads `placements.json` but not `default.json`.
- `playground/srcs/game_scene_widget.cpp:158-167` creates `WorldGenConfig` directly in C++.
- `playground/srcs/main.cpp:225-275` parses `--size` but uses it only for `--check-stairs` and `--map-only`; normal construction at `:296` passes only the seed.
- `playground/srcs/game_scene_widget.cpp:182-190` writes `playground/world_map.png` during ordinary startup.
- `playground/CMakeLists.txt:23-28` exports an absolute source resource path.

## Failure scenario

A developer edits `default.json` or runs the game with `--size 64` and observes no change. Moving the executable away from the checkout breaks resource lookup, while launching it in the checkout can modify a tracked file or fail on a read-only source tree.

## Proposed direction

Choose one authoritative configuration path and one runtime resource-location strategy. Either load and validate `default.json` or remove/migrate it. Apply supported CLI overrides consistently in every mode. Write generated previews to an explicit output/cache directory, and locate packaged resources relative to the executable or an overridable resource root.

## Acceptance criteria

- [ ] There is one documented source of truth for every world-generation setting.
- [ ] `--size` affects interactive mode or is rejected there with a clear message.
- [ ] Normal startup does not modify tracked repository files.
- [ ] The executable runs with resources from a relocatable or explicitly configured root.
- [ ] Generated-map output location is user-writable and reported to the user.
- [ ] Obsolete configuration is removed or migrated with schema validation.

## Tests

- [ ] Verify the same config is consumed by interactive, map-only, and stair-check modes.
- [ ] Launch from a working directory outside the source tree with a relocated resource bundle.
- [ ] Verify normal startup leaves the repository clean.
- [ ] Verify invalid config and unsupported CLI combinations fail clearly.

## Dependencies / notes

Coordinate the stable logical resource identity with Ticket 037. Configuration invariants themselves are defined by Ticket 008.
