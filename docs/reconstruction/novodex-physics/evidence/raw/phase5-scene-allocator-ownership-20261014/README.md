# Phase 5 Scene allocator packet raw evidence

Base revision: `5baf63fa`. The four implementation changes and target registration are the uncommitted packet under test; the restored candidate is the exact post-change tree. The staged-pair runner transcript records the oracle and candidate DLL hashes and paths.

- `before-fix-allocator-differential.log.gz`: first target run, six candidate cross-frees.
- The four `*-mutant-build.log.gz` / `*-mutant.log.gz` pairs rebuild and reject wrong-allocator routes for the condition object (`stdout_delta=10`), its worker state (`8`), the auxiliary object (`10`), and actor wrappers (`20`). Each mutant reports nonzero cross-frees. The patches are retained beside the runs.
- `restored-allocator-control.log.gz`: candidate matches oracle exactly (`stdout_delta=0`, exact stderr; 61 Foundation and 13 SDK allocations/frees, no cross-frees).
- `five-target-lifecycle-differential.log.gz`: SDK, actor lifecycle, Scene constructor, Scene allocator, and populated teardown controls.
- `phase5-gate.log.gz`: full Phase 5 gate, `2720/2720`.
- `phase7-gate.log.gz`: full downstream Scene/simulation gate with allocator ownership coverage, `1791/1791`.

The `postmerge-*` transcripts rerun both full gates from mainline at commit `fc841fe1`; Phase 5 passes 2,720/2,720 and Phase 7 passes 1,791/1,791. Earlier mutation and focused runs used the worktree's `build/scene-raycast-row700-red` Win32 Release build, `run_differential.ps1` with isolated temporary pair roots, and `run_phase_gate.ps1` for Phases 5 and 7. The evidence markdown records the result and the completion backlog limit.
