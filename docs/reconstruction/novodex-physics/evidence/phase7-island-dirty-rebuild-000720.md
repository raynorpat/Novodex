# Dirty island rebuild dispatch (`phys_fn_000720`)

Date: 2026-10-14
Row: `phys_fn_000720` at RVA `0x00016100`
Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential

The public simulation fixture's kinematic-to-dynamic transitions produced matching allocator and trajectory output when row 000720 was replaced with a no-op. The Scene path therefore remains supplementary evidence, not the row-specific detector: the later body update can rebuild the island as a fallback.

To isolate the row, the registered test calls the candidate DLL's mapped `Row000720Fixture::row000720` address (resolved from the staged `NxPhysics.map`) and the oracle at its pinned RVA. It seeds an aligned scratch root with the dirty flag set, then verifies that the row clears the dirty flag, marks traversal, and installs an island object. Baseline output is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`); both sides report `dirty_cleared=1 traversed=1 island=1 mallocs=4 frees=1 live_delta=3`.

In a clean Win32 Release archive, replacing only `Row000720Fixture::row000720` with an immediate return is detected by the registered differential: the candidate reports all three state checks false, exits 1, and the transcript differs by 4,000 bytes. Restored source has SHA-256 `4c02a1a7e5d7321d9bbead4b5c4b7e7cd5efdb4a1956d28d490891395734995a`, matching the pinned baseline source; the restored candidate differential is exact. The focused row and mutation patch are preserved under `evidence/raw/phase7-island-dirty-rebuild-000720-20261014/`.

The complete Phase 7 gate passes at 1,798/1,798 assertions in both the task worktree and after the fast-forward to main; the post-merge transcript is `evidence/raw/phase7-island-dirty-rebuild-000720-20261014/post-merge-phase7-gate.log.gz`. The mutation closes the row's direct rebuild contract; it does not claim that the public Scene transition fixture independently exposes this branch. Public headers remain unchanged.
