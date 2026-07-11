# Sparkle Playground

The playground is intentionally limited to three capabilities:

- streaming a generated voxel world around the player;
- exploring that world with click-to-move and orbit/zoom camera controls;
- composing and previewing voxel definitions in the voxel modeler.

The runtime executable is `SparklePlayground`. The editor executable is `EreliaTools`,
which now exposes only the Voxel Modeler page.

Battle, encounter, creature progression, AI, taming, and their editor pages are not part
of the playground. Their former implementation remains available in Git history.

Build and run instructions are in [howto/build-and-run.md](howto/build-and-run.md).

A layered walkthrough of the current codebase (big picture → systems → function-level,
plus cleanup proposals) lives in [code-tour/](code-tour/README.md).
