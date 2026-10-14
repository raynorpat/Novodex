# Island joint and contact solve dispatch (`phys_fn_000728`)

Date: 2026-10-14
Row: `phys_fn_000728` at RVA `0x000167c0`
Gate: registered Phase 7 `NxPhysicsSimulationTests` staged-pair differential

`Row000728Fixture::row000728` walks the island’s joint list, resets the joint solver words, dispatches each joint constraint, and then runs each retained contact pair. The public friction simulation exercises this path.

A clean Win32 Release archive produced an exact baseline (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Replacing only this row with an immediate return suppresses the joint and contact-pair loops; the registered differential rejects the mutant (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=6874`, `stderr_exact=False`). The candidate uses a distinct rebuilt DLL (SHA-256 `c79415ff0e0171a749c665c45be612fabb1c8668879ab30b123372a9a9c55f9a`).

The extracted archive source was restored byte-for-byte (SHA-256 `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`). MSBuild recompiled `Island.cpp`; the restored staged-pair differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

Raw configure/build/differential logs and the exact mutation patch are under `evidence/raw/phase7-island-constraint-contact-solve-000728-20261014/`. Row 000720 remains open: its no-op mutation had zero transcript delta, so current public scenarios did not exercise its dirty-island rebuild branch.
