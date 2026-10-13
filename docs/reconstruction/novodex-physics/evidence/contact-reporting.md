# Public actor contact reporting

This checkpoint wires actor-pair contact notifications through the rebuilt
simulation path. It covers the non-self actor-pair flag path for actors with a
single root shape (`phys_fn_000511`, `000513`, `000515`, `000589`, `000590`,
`000592`), contact-state refresh (`000905`), per-substep buffering
(`000917`/`000919`/`000921`), and fetch-side callback delivery (`000640`).

`tests/PhysicsSimulationTests.cpp` creates a dynamic sphere touching a static
plane under gravity, installs an `NxUserContactReport`, enables
`NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH` for the actor pair, and runs a
0.125-second step through `simulate`, `checkResults`, and `fetchResults`. The
oracle emits six callbacks: the first reports `0x0a`, followed by five `0x08`
touch reports. The callback actor identities and all force words match the
candidate. The public getter returns the configured `0x0a` flags on both sides.

Verification on the reconstruction worktree:

- `run_differential.ps1 -Targets NxPhysicsSimulationTests`: oracle and candidate
  both exit 0, `stdout_delta=0`, `stderr_exact=True`.
- `run_phase_gate.ps1 -Phase 7`: all registered targets pass; coverage evaluates
  `1243/1243` assertions.
- `run_phase_gate.ps1 -Phase 5`: all registered targets pass at `2037/2037`.
- `python -B -m unittest test_gate_targets`: 37 tests pass.
- `validate_inventory.py inventory.json`: passes with 6,338 functions,
  5,138 data objects, and zero unexplained records.

The separate fetch-dispatch fixture still injects oracle-layout trigger and
contact records to isolate `000640`. This checkpoint does not reconstruct
trigger-event generation, end-touch transitions, multi-shape actor-pair flag
expansion, actor-group pair flags, or same-actor error behavior. Those remain
open; the synthetic trigger fixture is not evidence for real trigger generation.

Row `phys_fn_000511`'s allocated actor-pair clear path is now independently
covered: the registered pair-flag fixture stores `NX_NOTIFY_ON_TOUCH`, clears
the pair with zero flags, and checks the getter returns zero. A no-op mutation
leaves `0x8` visible and is rejected (`stdout_delta=5`); the restored control is
exact. See [the row-specific evidence](phase7-contact-pair-flags-clear-000511.md).

The same fixture now exercises Scene getter rows `phys_fn_000513` and
`phys_fn_000515`. Same-shape and same-actor calls must each return zero and
emit their exact invalid-parameter diagnostic; separate no-op mutations are
rejected with `stdout_delta=23` and `stdout_delta=11`. See
[the getter evidence](phase7-contact-pair-flag-getters-000513-000515.md).

The same registered target covers setter rows `phys_fn_000589`, `000590`, and
`000592`: valid record update/clear, the same-shape setter diagnostic, and the
same-actor setter diagnostic. A no-op mutant for each row is rejected with
`stdout_delta=23`, `26`, and `15`; after restoration the focused differential
is exact. Row 000590 now explicitly calls the recovered const-byte-pointer
implementation of 000589, avoiding an older overload that left the oracle row
unreachable. See [the setter evidence](phase7-contact-pair-flag-setters-000589-000590-000592.md).
