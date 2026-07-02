# How-to — Add tests (and what to hand-validate instead)

Two test suites, one convention.

## Where tests go

- **PlaygroundTests** (`Playground/tests/`, target from step 01) — all `pg::` logic. Files
  mirror `srcs/` (`Playground/tests/battle/turn_rules_test.cpp` tests
  `srcs/battle/rules/...`). gtest, same style as the spk suite
  (`tests/TUs/srcs/structures/math/matrix_test.cpp` is the style reference: plain `TEST`,
  `expectNear` helpers, no fixtures unless shared setup is real).
- **SparkleTests** (`tests/TUs/`) — only for `spk::` code, i.e. promotion steps. Never link
  `pg::` into it.

## What gets a test vs. a hand-validated visual

| Test (headless, required) | Hand-validate (screenshot to the user) |
|---|---|
| rules, math, parsing, registries | anything rendered: meshes, lighting, overlays |
| graph building, A*, LoS, raycasts | camera framing/blending feel |
| turn/feat/taming state machines | HUD layout, hover states |
| event emission (log contents!) | animation look, audio |
| save round-trips, determinism | tool UX |

The dividing rule: if the assertion is a number or a state, test it; if it's "does it look
right", the user validates ([memory: expected images are hand-validated only]).
Design for it: logic that computes lives in plain functions/classes the test calls
directly; ComponentLogic/widgets stay thin adapters.

## Battle-test workhorse: BoardFixture

`Playground/tests/support/board_fixture.hpp` (built in step 09): builds a `BoardData` from
ASCII layer strings (`#` cube, `.` empty, `/` slope+Z, `s` stair, `b` bush…), returns named
cells. Every board/battle test uses it — extend its legend there, not per-test.

```cpp
TEST(Pathfinder, ClimbsSlopeChain)
{
    pg::test::BoardFixture fixture({ "####", "##/#", "####" }, /*legend defaults*/);
    auto path = pg::Pathfinder::findPath(fixture.graph(), fixture.cell(0,0), fixture.cell(3,0), {});
    ASSERT_TRUE(path.has_value());
}
```

## Determinism tests

Anything seeded (encounters, world gen) asserts *exact* outputs for a fixed seed. When a
legitimate change breaks them, regenerate the expectation **deliberately** in the same
commit and say so in the message.

## Commands

`[test]` for PlaygroundTests, `[spk-test]` for the library suite
([build-and-run.md](build-and-run.md)). Both green before a step is Done.
