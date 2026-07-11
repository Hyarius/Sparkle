# Sparkle engineering backlog

This directory turns review findings into small, durable engineering tickets. The folder containing a ticket is its workflow state:

```text
backlog/
├── README.md             # This workflow guide
├── ticket_template.md    # Copy when creating a ticket
├── open/                 # Ready, in-progress, or blocked work
└── complete/             # Finished tickets with verified acceptance criteria
```

The numbered filename is the stable ticket ID. Keep it unchanged when editing or moving the ticket.

## Pick up a ticket

1. Browse `backlog/open/` and choose one numbered ticket.
2. Read its problem, evidence, acceptance criteria, tests, and dependencies before changing code.
3. Change `Status` from `Ready` to `In Progress` and optionally add yourself as owner.
4. Implement only the ticket's intended outcome. Record any material scope decision in `Dependencies / notes`.
5. Run the ticket's test checklist and any broader suite appropriate to the affected subsystem.
6. Check every satisfied acceptance/test item. Do not check an item based only on code inspection when it asks for runtime or test evidence.
7. Set `Status` to `Complete` and move the same file:

   ```powershell
   git mv backlog/open/NNN-short-title.md backlog/complete/NNN-short-title.md
   ```

8. Commit the completed ticket with the implementation and tests so the historical record stays tied to the change.

A blocked ticket remains in `open/` with `Status: Blocked` and a concise explanation of the blocker. Moving a file is reserved for completed work.

## Definition of done

A ticket may move to `complete/` only when:

- every acceptance criterion is satisfied or explicitly superseded with a documented reason;
- required regression tests exist and pass;
- relevant existing tests and builds pass;
- public contracts, data schemas, and comments affected by the change are updated;
- no temporary diagnostics, generated output, or unrelated edits remain;
- the ticket records any follow-up work that was intentionally left out of scope.

## Create a ticket

1. Choose the next unused three-digit ID. IDs are never recycled.
2. Copy the template into `open/` with a concise kebab-case filename:

   ```powershell
   Copy-Item backlog/ticket_template.md backlog/open/041-short-imperative-title.md
   ```

3. Replace every placeholder. A ticket must be understandable without the original review or conversation.
4. Cite concrete repository paths and line numbers under `Evidence`.
5. Make acceptance criteria observable and tests specific enough that another contributor can decide whether the work is finished.
6. Keep one primary problem per ticket. If investigation reveals independently deliverable work, create linked tickets instead of silently expanding scope.

## Metadata conventions

### Status

- `Ready` — sufficiently described and available to pick up.
- `In Progress` — implementation is active.
- `Blocked` — work cannot continue; explain why in the ticket.
- `Complete` — acceptance criteria and tests are verified; the file belongs in `complete/`.

### Priority

- `P0 — Critical` — memory safety, data corruption, security, or a release-blocking failure.
- `P1 — High` — major correctness defect or severe current-path performance failure.
- `P2 — Medium` — bounded defect, important robustness gap, or architectural risk.
- `P3 — Low` — cleanup, documentation drift, or minor hardening.

Priority describes impact, not implementation order. Dependencies and shared architectural work may determine the practical sequence.

## Ticket-writing guidance

- Describe observed or demonstrable behavior, not presumed author intent.
- Distinguish a current production path from a conditional public-API hazard.
- Keep proposed direction flexible when several designs could satisfy the contract.
- Prefer acceptance criteria phrased as externally visible invariants.
- Include negative, boundary, and failure-path tests where relevant.
- Preserve completed tickets; they are lightweight architecture and maintenance history.
