# `NxBodyStep` island sleep/wake state update mutation proof

Date: 2026-10-14
Row: `phys_fn_000732` at RVA `0x00016860`
Gate: registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential

`Row000732Fixture::row000732` updates saved and live velocity state around the island sleep/wake decision. A clean Win32 Release archive and the existing public simulation sleep fixture provide a row-specific differential.

The baseline run passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. In a throwaway archive, replacing only this row with an immediate return changes the automatic sleep observation from `sleeping=1` to `sleeping=0`; the registered differential rejects the mutant with `oracle_exit=0`, `candidate_exit=1`, and `stdout_delta=6720`. The following wake case is also present in the fixture.

The mutation was restored byte-for-byte (`BodyStep.cpp` SHA-256 `23cbd7af9238220ec8db661d68ef69fb088f28314ece7643aedb6cfb87da42ca`). The restored Release DLL SHA-256 `18bbd63d1ad8691eaca5694bb361585c04889c04615ff03784a4600e95392724` matches the baseline, and the restored differential is exact (`stdout_delta=0`, `stderr_exact=True`).

The fresh full Phase 7 gate passes at 1,793/1,793 coverage assertions; all 29 registered staged-pair targets pass exact differentials, and the row’s `NxPhysicsSimulationTests` differential is exact. The Phase 7 gate also passed after commit `4cac0bd8` was fast-forwarded to `main`; the post-merge gate transcript is archived alongside the clean-build, baseline, mutant, and restore logs, exact mutation patch, and checksums under `evidence/raw/phase7-body-sleep-wake-000732-20261014/`. This closes the exercised row 000732 only; caller 000714 and the remaining simulation pipeline stay open.
