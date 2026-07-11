# Bound and overflow-proof streamer window expansion

- **Status:** In Progress
- **Priority:** P1 — High, availability and undefined behavior
- **Area:** Engine / voxel chunk streaming / coordinate safety

## Problem

Streamer windows are expanded with signed `int` arithmetic using `origin - range`, `origin + range`, and inclusive loops. View ranges are checked only for negativity; no upper bound, checked volume, or coordinate-overflow validation exists.

An origin near `INT_MIN`/`INT_MAX` can overflow while computing a bound or incrementing the inclusive loop variable. Even `origin.x == INT_MAX` with `range.x == 0` reaches `x++` after processing the one intended coordinate. Less extreme but very large valid ranges can synchronously construct an enormous `unordered_set`, generate/load every requested chunk in one update, and stall or exhaust memory. Work is not distance-prioritized, incremental, or cancellable.

## Evidence

- View range validation rejects negative components but sets no practical/representable upper bound: [`srcs/structures/voxel/spk_voxel_chunk_streamer.cpp:12`](../../srcs/structures/voxel/spk_voxel_chunk_streamer.cpp#L12).
- Window bounds and inclusive increments use unchecked signed arithmetic: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:47`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L47).
- Every wanted coordinate is inserted before execution begins: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:49`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L49).
- Every wanted chunk is then synchronously obtained/generated in the same update: [`srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp:74`](../../srcs/structures/voxel/spk_voxel_chunk_streamer_logic.cpp#L74).

## Failure scenario

1. Set a streamer origin component to `INT_MAX` and its corresponding view range to zero.
2. The inclusive loop processes `INT_MAX`, then increments the signed loop variable past its maximum, invoking undefined behavior.

Alternatively, set a large nonnegative range. The code attempts to enumerate `(2x+1)(2y+1)(2z+1)` coordinates and load them in one frame, causing a long stall or allocation failure despite the setter accepting the value.

## Proposed direction

Validate the complete requested window with checked or wider arithmetic before iterating. Establish a practical maximum volume and a clear error/result for unrepresentable bounds. For ranges large enough to be useful but too expensive for one frame, process loading/unloading incrementally with a configurable budget and a deterministic priority such as distance from the nearest streamer.

Use iteration constructs that do not increment beyond the inclusive maximum. Preserve union behavior for multiple streamers without materializing unsafe intermediate volumes.

## Acceptance criteria

- [ ] No valid call path performs signed overflow when computing or iterating a streamer window.
- [ ] Origins at every integer boundary have defined behavior, including range zero.
- [ ] Requested window volume is checked without overflowing its calculation type.
- [ ] Excessive/unrepresentable ranges are rejected with an actionable error or handled by a documented bounded strategy.
- [ ] Streaming work per update has a defined bound if large supported windows remain allowed.
- [ ] Incremental loading, if selected, prioritizes coordinates deterministically and responds correctly when the streamer moves before completion.
- [ ] Multiple-streamer union semantics remain correct.

## Tests

- [ ] Test `INT_MIN`, `INT_MAX`, and adjacent origins with zero and small ranges on every axis.
- [ ] Test the largest accepted range and the first rejected range.
- [ ] Test overflow of each bound and of the 3-D volume calculation.
- [ ] Verify an excessive request fails before inserting coordinates or loading chunks.
- [ ] If work is incremental, assert the per-frame cap and eventual completion.
- [ ] Move a streamer while a large window is pending and verify obsolete work is cancelled/deprioritized.
- [ ] Run boundary tests with undefined-behavior sanitization where supported.

## Dependencies / notes

This ticket concerns safe range expansion and bounded work. The zero-streamer cleanup and historical map-state issues are tracked separately in ticket 009.
