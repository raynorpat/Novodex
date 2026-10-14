# Task M6.21 brief — island joint/contact solver dispatch (`phys_fn_000728`)

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `fe81254c` plus the M6.20 evidence/selection notes

## Contract

Falsify reconstructed `Row000728Fixture::row000728` at RVA `0x167c0`, covering its per-island joint constraint and contact-pair solver dispatch through registered Phase 7 `NxPhysicsSimulationTests`.

## Proof result

An immediate-return mutation is rejected by the friction simulation (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`). Restoring the extracted source byte-for-byte and forcing the MSBuild recompilation returns the differential to exact (`stdout_delta=0`, `stderr_exact=True`).

## Remaining checks

Complete: 831 tool tests and 742 subtests pass; inventory/backlog validate; both public-header manifests pass; post-merge full Phase 7 gate on main passes at 1,793/1,793. Raw build/differential logs, post-merge gate transcript, and mutation patch are retained. Row 000720 remains open.
