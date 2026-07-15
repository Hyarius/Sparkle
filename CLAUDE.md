# Sparkle

The `spk::` engine (`includes/`, `srcs/`) plus **Playground** (`Playground/`), a `pg::`
testbed. The active work is the Erelia battle mode, built as an ordered plan under
`Playground/docs/plan/`.

## Implementing a Playground battle plan step

The plan is `Playground/docs/plan/steps/step-NN-*.md`, implemented in order.

**Before implementing a step, read [`Playground/docs/plan/PROGRESS.md`](Playground/docs/plan/PROGRESS.md).**
It is the progress ledger: the real public API surface (files, signatures, invariants) of
every completed step, so you do not re-read all their headers to understand what earlier
steps built. Read the ledger for steps `1..N-1`, then the step-N prompt, then only the live
files step N names. Open an earlier header only for a detail the ledger omits.

**After finishing a step, refresh that step's section in `PROGRESS.md`** from the headers the
step actually landed — compact: files added, key public signatures, invariants. The ledger
is only worth trusting if it stays current; the next step will rely on it.

If the ledger and the code disagree, the code wins and the ledger should be corrected.
