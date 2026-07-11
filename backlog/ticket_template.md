# Ticket NNN — Short imperative title

- **Status:** Ready
- **Priority:** P2 — Medium
- **Area:** Engine / subsystem
- **Owner:** Unassigned

## Problem

Describe one concrete defect, fragility, or missing capability. State the violated invariant and distinguish current-path impact from conditional API risk.

## Evidence

- `path/to/file.cpp:123` — explain what this code establishes.
- `path/to/related_header.hpp:45` — cite the relevant public contract or missing guard.
- Add test, log, profiler, or reproduction evidence when available.

## Failure scenario

Give the smallest realistic sequence or input that exposes the problem. Include the resulting user-visible behavior, incorrect state, exception, undefined behavior, or performance cost.

## Proposed direction

Describe the desired design properties and likely implementation boundary. Avoid over-prescribing a mechanism when multiple designs could meet the acceptance criteria.

## Acceptance criteria

- [ ] State an observable invariant that must hold after the change.
- [ ] Cover the original failure scenario.
- [ ] Preserve an important existing behavior or compatibility contract.
- [ ] Update public documentation, comments, or data schemas affected by the change.
- [ ] Ensure failure paths produce a clear, appropriately typed diagnostic where applicable.

## Tests

- [ ] Add a focused regression test that fails before the fix and passes afterward.
- [ ] Add relevant boundary, invalid-input, lifecycle, or concurrency coverage.
- [ ] Run the affected subsystem suite.
- [ ] Run broader build/integration checks appropriate to the risk.

## Dependencies / notes

List prerequisite or follow-up ticket IDs, scope exclusions, migration concerns, and important implementation decisions. Write `None` when there are no known dependencies.
