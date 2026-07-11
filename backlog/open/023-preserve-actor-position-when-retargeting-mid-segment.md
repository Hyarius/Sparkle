# Preserve actor position when retargeting mid-segment

- **Status:** Ready
- **Priority:** P2 — Medium
- Area: Playground / Actor navigation

## Problem

Movement state separates the actor's last completed logical cell from its interpolated position along the current path segment. A new move request pathfinds from the last completed cell, replaces the path, and resets segment progress to zero. The next update therefore interpolates from the old cell center again, visibly snapping the actor backward.

An invalid target clears the path and progress. The normal end-of-path branch then calls `placeAtCell()`, which can also snap a moving actor back to its previous completed cell.

## Evidence

- `playground/srcs/logics/actor_path_logic.cpp:79-92` pathfinds from `p_actor.cell` and unconditionally resets segment state.
- `playground/srcs/logics/actor_path_logic.cpp:81` does not account for the current interpolated position or next waypoint.
- `playground/srcs/logics/actor_path_logic.cpp:83-87` clears movement state on an invalid target.
- `playground/srcs/logics/actor_path_logic.cpp:105-113` computes and applies the interpolated position from the stored path segment.
- `playground/srcs/logics/actor_path_logic.cpp:116-118` updates `actor.cell` only after completing a segment.
- `playground/srcs/logics/actor_path_logic.cpp:125-130` snaps completed or cleared paths back to the logical cell center.

## Failure scenario

1. Begin moving from cell A to adjacent cell B.
2. At roughly 80% progress, request a path to C.
3. The new path starts at A and progress becomes zero.
4. On the next update, the actor is placed near A instead of continuing from its current world position.

The same backward jump occurs when the new target is invalid and the path is cleared.

## Proposed direction

Retarget from a continuous movement state. Possible implementations include preserving the active segment until an endpoint and routing from that endpoint, or adding a transient first segment from the actor's current foot position into the new cell path. Whichever behavior is selected should:

- avoid changing the actor's transform at request time;
- never move backward solely because a route was replaced;
- define whether an invalid target stops in place or finishes the current segment;
- keep the logical cell, collision/traversal state, facing, and player-moved events coherent.

## Acceptance criteria

- [ ] A valid retarget during a segment causes no visible position discontinuity.
- [ ] An invalid retarget causes no snap to the previous cell.
- [ ] The actor's logical cell is always a valid traversal anchor under the chosen transition policy.
- [ ] Facing and `playerMoved` events remain correct through a retarget.
- [ ] Requests made exactly at a segment boundary retain existing behavior.
- [ ] The retargeting policy is documented in `ActorPathLogic`.

## Tests

- [ ] Retarget at 25%, 50%, and 90% of a horizontal segment and assert position continuity within an epsilon.
- [ ] Repeat on a vertical/sloped segment.
- [ ] Request an unreachable target mid-segment and verify the chosen stop behavior.
- [ ] Retarget at an endpoint and verify no duplicate or missing movement event.
- [ ] Cover a large frame delta that crosses a segment near the retarget.

## Dependencies / notes

The solution may require representing a transient world-space segment separately from graph-cell paths. Avoid changing pathfinding semantics as part of this ticket unless necessary.
