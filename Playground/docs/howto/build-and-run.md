# How-to — Build, Run, Test

All commands from the repo root (`c:\Users\User\source\repos\Sparkle`), PowerShell or bash.

## Configure (once per preset, or after CMakeLists/vcpkg changes)

```
cmake --preset playground          # game + PlaygroundCore + PlaygroundTests + EreliaTools
cmake --preset test                # spk:: library tests (SparkleTests)
```

vcpkg runs through the preset toolchain automatically; adding a dependency = edit
`vcpkg.json` + reconfigure.

## Build / run / test tags used by the plan steps

| Tag | Command |
|---|---|
| `[build]` | `cmake --build build/playground --target SparklePlayground` |
| `[run]` | `./build/playground/Playground/SparklePlayground.exe --resource-path Playground/resources` (path may vary by generator — check the build output; without `--resource-path`, resources resolve to `resources/` next to the exe; cwd doesn't matter for anything else) |
| `[test]` | `cmake --build build/playground --target PlaygroundTests && ./build/playground/Playground/PlaygroundTests.exe` |
| `[tools]` | `cmake --build build/playground --target EreliaTools && ./build/playground/Playground/EreliaTools.exe` |
| `[spk-test]` | `cmake --build build/test --target SparkleTests && ctest --test-dir build/test` (required green for promotion steps; run after any `spk::` change) |

Before step 01 lands, only `[build]`/`[run]` exist (no Playground tests target).

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
