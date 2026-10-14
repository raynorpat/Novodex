# Task M6.21 brief — island joint/contact solver dispatch (`phys_fn_000728`)

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `fe81254c` plus the M6.20 evidence/selection notes

## Contract

Falsify reconstructed `Row000728Fixture::row000728` at RVA `0x167c0`, covering its per-island joint constraint and contact-pair solver dispatch through registered Phase 7 `NxPhysicsSimulationTests`.

## Proof result

An immediate-return mutation is rejected by the friction simulation (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`). Restoring the extracted source byte-for-byte and forcing the MSBuild recompilation returns the differential to exact (`stdout_delta=0`, `stderr_exact=True`).

## Remaining checks

Run tool tests, inventory/backlog/header checks, and full Phase 7 gate on main after merge. Retain raw build/differential logs and mutation patch. Row 000720 remains open.
