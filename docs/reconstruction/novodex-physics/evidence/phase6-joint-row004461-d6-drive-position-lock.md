# Phase 6 closure: D6 drive-position write-lock path (`phys_fn_004461`)

`phys_fn_004461` at RVA `0x000b0a50` is `NpD6Joint::setDrivePosition`. Its folded internal drive setter is empty, so a normal setter call cannot prove that the wrapper ran. The staged-pair fixture now also marks the scene lock as held by another thread and calls the public setter. The expected output stream report is one `NXE_INVALID_OPERATION` (`code=2`) at source line 40 (`0x28`).

The baseline matches the pinned oracle exactly: both processes exit zero, `stdout_delta=0`, and stderr matches. Temporarily suppressing this row's `reportWriteLocked` call removes the expected report; the registered `NxPhysicsJointStagedPairTests` differential catches the mutation with both processes exiting zero, `stdout_delta=24`, and exact stderr. Restoring the implementation returns the differential to zero.

The mutation was isolated to the worktree and restored before the exact control run. The fixture adds one registered assertion to each of the Phase 6 and Phase 7 coverage gates. The public headers are unchanged.

Build root: `D:\github\Novodex\build\phase6-joint-004095-20261009`.

Logs:

- `joint-004461-baseline.log` — contention fixture baseline, exact output.
- `joint-004461-mutation.log` — suppressed-report mutation, `stdout_delta=24`.
- `joint-004461-restored.log` — restored exact control.

The oracle Physics DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutated candidate Physics DLL SHA-256 is `29e4ffb3e55b5c6aa1919e3e25c536416b18150aeff6555a2562dc3ab4a5b6ad`; restored candidate SHA-256 is `c6ba2a2603dba3a9f84e8771f22ae6e1485ce67132767c0795ec9bc91b45cf99`.
