# Step 01 — Build split + nlohmann-json + JSON loader & registries

**Phase A · critical path · no dependencies**

## Goal

Turn Playground into a testable multi-target project (D17) and give it the strict JSON
loading machinery every later step authors data with (D02/D10).

## Reading

[01-architecture.md §7/§8](../../01-architecture.md) ·
[02-data-model.md §1/§16](../../02-data-model.md) ·
[howto/add-json-definition.md](../../howto/add-json-definition.md) ·
current `Playground/CMakeLists.txt`, root `vcpkg.json`, `tests/TUs/CMakeLists.txt` (copy its
gtest wiring style).

## Files

- **Modify** `vcpkg.json` — add `"nlohmann-json"`.
- **Rewrite** `Playground/CMakeLists.txt`:
  - `PlaygroundCore` static lib = glob `srcs/**` minus `srcs/main.cpp`; links
    `Sparkle::Sparkle` + `nlohmann_json::nlohmann_json` PUBLIC; `PG_RESOURCE_DIR` compile
    definition moves here (PUBLIC); include dir `srcs/` PUBLIC; C++23.
  - `SparklePlayground` exe = `srcs/main.cpp`, links `PlaygroundCore`.
  - `PlaygroundTests` exe = glob `Playground/tests/**`, links `PlaygroundCore` +
    `GTest::gtest_main` (vcpkg gtest already present).
  - (`EreliaTools` target arrives in step 25 — leave a comment placeholder.)
- **Create** `Playground/srcs/core/json.hpp/.cpp` — `pg::JsonError` (std::runtime_error with
  file/path/message), `pg::JsonLoader::parseFile`, `pg::JsonReader`
  (`require<T>/optional<T>/requireEnum/child/childArray/forbidUnknown`, path tracking).
- **Create** `Playground/srcs/core/registry.hpp` — `pg::Registry<T>` (load(dir, parseFn),
  get/tryGet/ids; sorted filename iteration), `pg::PolymorphicFactory<TBase>`.
- **Create** `Playground/srcs/core/game_rules.hpp/.cpp` — `pg::GameRules` struct + parser
  for `config/game-rules.json`; a global-ish `const GameRules&` owned by the registries
  bundle.
- **Create** `Playground/srcs/core/registries.hpp/.cpp` — `pg::Registries` aggregate with
  `loadAll(dataDir)` in the documented load order (only `config` exists yet; the function
  grows each data step).
- **Create** `Playground/resources/data/config/game-rules.json` (values from
  [02-data-model.md §2](../../02-data-model.md)).
- **Create** `Playground/tests/core/json_reader_test.cpp`, `registry_test.cpp`,
  `Playground/tests/main.cpp` (gtest main if not using gtest_main).

## Work items

1. vcpkg + CMake restructure; reconfigure; all three targets build; the game still runs
   (cube scene untouched).
2. JSON machinery per [02-data-model.md §16](../../02-data-model.md) — error messages must
   carry `file:jsonPath` (e.g. `voxels/grass.json:$.shape.type`).
3. GameRules load; `main.cpp` loads Registries before the widget and aborts cleanly on error
   (print message, return non-zero).

## Tests (`[test]`)

JsonReader: require/optional/enum happy paths; missing key, unknown key (forbidUnknown), bad
type, bad enum — each asserts the thrown message contains the path. Registry: loads two
in-memory files (use a temp dir via `std::filesystem`), id = stem, sorted order, duplicate
id error, dangling-directory error. GameRules: sample parse + unknown-field rejection.

## Definition of Done

- `[build]`, `[test]`, `[run]` all work; cube scene renders as before.
- Deliberately corrupting `game-rules.json` (unknown key) makes `[run]` abort with a message
  naming the file and key — demonstrate in the step notes.
- No `spk::` changes.
