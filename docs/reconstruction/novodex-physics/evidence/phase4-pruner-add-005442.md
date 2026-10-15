# Phase 4 Pruner process-pool add: `phys_fn_005442`

`NxPrunerProcessPool::add` at RVA `0x000ef3d0` allocates a registration slot and returns its generation-tagged handle. The existing registered `NxPhysicsSceneRaycastTests` fixture inspects the actual DLL-owned type-0 and dynamic Pruner registration words, requiring static `00000000.00000006` and dynamic `00000001.00000004`.

For row-specific falsification, an unconditional return of handle 0 was inserted at function entry and the Win32 Release `NxPhysics.dll` was rebuilt. The oracle retained the expected registrations; the candidate reported dynamic `00000000.00000004` and failed the assertion (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2794`). Restoring the source, touching it to force MSBuild recompilation, and rebuilding returned the staged-pair differential to exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

The user-approved Phase 6 date/time assertion was also rerun in this build: `NxPhysicsCoreDumpTests` validates the generated fields before normalizing the clock line, and its restored staged pair is exact. This run supplements the entry-return mutation evidence in [phase6-scene-dump-datetime-003992.md](phase6-scene-dump-datetime-003992.md).

The pruner-add mutation, restored build and differential, and date/time run are retained with hashes in [the raw evidence directory](raw/phase4-pruner-add-005442-20261015/). Capacity growth, slot reuse under larger live sets, and allocation failure remain open behavior for the neighboring pool rows.
