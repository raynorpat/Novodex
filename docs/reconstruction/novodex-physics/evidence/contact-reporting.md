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
