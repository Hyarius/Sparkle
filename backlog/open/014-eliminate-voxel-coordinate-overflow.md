# Eliminate signed overflow in voxel coordinate operations

- **Status:** Ready
- **Priority:** P1 — High
- Area: Voxel coordinates / Prefabs / Robustness

## Problem

Voxel coordinate operations use signed int arithmetic without a defined supported domain or overflow checks. Multiplication, subtraction, addition, unary negation, inclusive loop increments, and extent calculations can overflow. Several affected functions are noexcept, turning an invalid external coordinate into undefined behavior rather than a recoverable error.

The risk is spread across chunks, prefab rotation and sizing, map stamping, neighbor coordinates, and inclusive range construction. INT_MIN is especially unsafe under quarter-turn rotation, while ranges ending at INT_MAX can overflow their loop counter and fail to terminate.

## Evidence

- srcs/structures/voxel/spk_voxel_chunk.cpp:40 — chunk coordinates are multiplied by fixed chunk size in worldOrigin().
- srcs/structures/voxel/spk_voxel_chunk.cpp:45 — local coordinates subtract the potentially overflowing world origin.
- srcs/structures/voxel/spk_prefab.cpp:42 — noexcept rotation negates signed coordinates.
- srcs/structures/voxel/spk_prefab.cpp:113 — bounds are updated before inclusive range iteration.
- srcs/structures/voxel/spk_prefab.cpp:114 — inclusive int loops can overflow when a maximum is INT_MAX.
- srcs/structures/voxel/spk_prefab.cpp:146 — size computes max - min + 1 in noexcept signed arithmetic.
- srcs/structures/voxel/spk_prefab.cpp:163 — rotated bounds subtract the pivot and negate components.
- srcs/structures/voxel/spk_prefab.cpp:175 — application adds destination to a rotated relative coordinate.
- srcs/structures/voxel/spk_voxel_map.cpp:229 — prefab bounds are added to the world destination before chunk conversion.
- srcs/structures/voxel/spk_voxel_map.cpp:232 — inclusive chunk loops have the same terminal-increment hazard.

## Failure scenario

Add a voxel range whose maximum X is INT_MAX, rotate a prefab containing INT_MIN, or ask for worldOrigin() on a chunk coordinate beyond INT_MAX divided by chunk size. Depending on optimization, the operation can wrap, loop indefinitely, address an unrelated chunk, or corrupt bounds and allocations.

## Proposed direction

Define the valid coordinate domain and centralize checked vector arithmetic. Perform intermediate calculations in a wider type, verify the result fits the public coordinate type, and throw a specific exception before mutation when it does not. Replace overflow-prone inclusive loops with checked extents, wider counters, or termination logic that never increments past the endpoint. Remove noexcept where failure becomes part of the contract.

## Acceptance criteria

- [ ] The supported world, chunk, local, pivot, and prefab extent ranges are documented.
- [ ] All coordinate add, subtract, multiply, negate, and inclusive-extent operations either return a representable value or fail before mutation.
- [ ] No loop requires incrementing INT_MAX to terminate.
- [ ] Prefab bounds are not updated until range capacity and iteration bounds have been validated.
- [ ] Invalid orientation values and coordinate overflows produce consistent, diagnosable errors.
- [ ] Normal negative-world-coordinate behavior remains unchanged.

## Tests

- [ ] Add boundary tests around INT_MIN, INT_MAX, and chunk-size multiplication limits.
- [ ] Add prefab rotation tests containing the most-negative representable components.
- [ ] Add inclusive range tests ending at INT_MAX on each axis.
- [ ] Add destination-plus-bounds overflow tests for map stamping.
- [ ] Run the coordinate suite with signed-integer undefined-behavior sanitization where supported.

## Dependencies / notes

This should provide shared checked helpers for streamer range handling and world-generation arithmetic rather than introducing isolated fixes. Changing the public coordinate type is a larger compatibility decision; checked wider intermediates can be delivered first.
