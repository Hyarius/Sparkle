# How-to — Build, Run, Test

All commands from the repo root (`c:\Users\User\source\repos\Sparkle`), PowerShell or bash.

## Configure (once per preset, or after CMakeLists/vcpkg changes)

```
cmake --preset test                # Sparkle tests + Playground tests + SparklePlayground
cmake --preset playground          # SparklePlayground only, Debug
cmake --preset playground-release   # SparklePlayground only, Release
```

`test` is the preset the plan steps use: it sets `SPARKLE_BUILD_TESTS=ON`, which builds
`SparkleTests`, `PlaygroundCore`, `SparklePlayground` **and** `PlaygroundTests`. The
`playground` presets build the game alone and have no test target.

vcpkg runs through the preset toolchain automatically; adding a dependency = edit
`vcpkg.json` + reconfigure.

## Build / run / test tags used by the plan steps

| Tag | Command |
|---|---|
| `[build]` | `cmake --build build/test --target SparklePlayground` |
| `[run]` | `./build/test/Playground/SparklePlayground.exe --resource-path Playground/resources` |
| `[test]` | `cmake --build build/test --target PlaygroundTests SparklePlayground` then `ctest --test-dir build/test --output-on-failure -L playground` |
| `[spk-test]` | `cmake --build build/test --target SparkleTests && ctest --test-dir build/test` (required green for promotion steps; run after any `spk::` change) |

`ctest -L playground` is the canonical Playground test path: it runs the GTest cases **and**
the CLI tests, which the raw executable does not. While debugging one case, running the
binary directly is quicker:

```
./build/test/Playground/tests/PlaygroundTests.exe --gtest_filter=BattleTimeTest.*
```

Paths above are the Ninja layout of the `test` preset. Other generators nest binaries under
a configuration folder — check the build output rather than assuming.

## Resources are never copied

`pg::resourceRoot()` resolves to the `--resource-path` override when one is given, else to a
`resources` folder next to the executable. Nothing is copied at build time, so a run without
`--resource-path` aborts with "resources not found" unless you deployed one yourself. Point
it at the source tree (`--resource-path Playground/resources`) and data edits apply with no
rebuild. `PlaygroundTests` pins its own lookup at the source tree through
`PG_TEST_RESOURCE_DIR`, so tests need no flag.

## Smoke-run convention

For steps with a visual Definition of Done: `[run]`, exercise the listed scenario, screenshot
(Win+Shift+S or ask the user), and **have the user validate** — visual results are
hand-validated per project convention; tests only cover headless logic
([add-tests.md](add-tests.md)).

## Troubleshooting

- Linker errors after adding files: the CMake `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`
  usually re-globs on build; if a new file is missed, reconfigure the preset.
- GL errors print to stderr; run from a terminal to see them.
- A JSON load error aborts startup by design (D10) — the message names file + field.
