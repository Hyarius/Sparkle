# Sparkle Playground

The playground is intentionally limited to three capabilities:

- streaming a generated voxel world around the player;
- exploring that world with click-to-move and orbit/zoom camera controls;
- composing and previewing voxel definitions in the voxel modeler.

The only executable is `SparklePlayground`; `PlaygroundTests` is its headless test binary.
There is no `EreliaTools` target in the checked-in CMake graph — docs that still describe
one (`03-systems/tooling.md`, `howto/add-tool-tab.md`) describe a removed subsystem.

Battle, encounter, creature progression, AI, and taming are being rebuilt from
[plan/steps/](plan/steps/README.md). Step 01 has landed the shared foundations only —
fixed-point battle time, stable IDs, the deterministic RNG, the common enums and the
game-rules v2 schema ([03-systems/battle-foundations.md](03-systems/battle-foundations.md)).
No board, session, scheduler, command, effect, AI, HUD or save format exists yet.

Build and run instructions are in [howto/build-and-run.md](howto/build-and-run.md).

A layered walkthrough of the current codebase (big picture → systems → function-level,
plus cleanup proposals) lives in [code-tour/](code-tour/README.md).
