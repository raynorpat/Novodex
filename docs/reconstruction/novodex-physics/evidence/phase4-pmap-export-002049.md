# `NxCreatePMap` export wrapper (`phys_fn_002049`)

`NxCreatePMap` is now mapped to `Physics/src/PMap.cpp` and exercised through
the exported DLL entry point by `NxPhysicsTriangleMeshApiTests`. The authored
tetrahedron compute case pins the density-32 result at 10,444 bytes with hash
`9a70de00aaf0edd4`. Phase 4 requires this exact result line from the staged
oracle and candidate processes.

The row-specific mutation inserted an immediate `false` return at the start of
the wrapper, suppressing its call into `PenetrationMap::create`, stream
collapse, and public output stores. The oracle retained the pinned result while
the candidate printed `created=0 size=0 hash=0000000000000000`; the registered
differential caught it with `stdout_delta=26` (oracle exit 0, candidate exit 1).
After restoring the wrapper, the oracle and candidate both printed the pinned
result, exited 0, and matched with `stdout_delta=0` and exact stderr.

The restored test also continues through PMap load/export and the triangle-mesh
API matrix. This closes the exported wrapper row only. `PenetrationMap::create`
(`phys_fn_002047`) owns the compute algorithm; its load-arm closure does not
establish the compute body, which remains independently tracked.

Retained raw transcripts:

- [`pmap-export-mutation.log`](raw/phase4-pmap-export-002049/pmap-export-mutation.log)
- [`pmap-export-restored.log`](raw/phase4-pmap-export-002049/pmap-export-restored.log)

The fresh full Phase 4 gate passes after evaluating 519 assertions against the then-current 518 floor. The registry floor is now 519, and the floor-to-registration unit test passes. Both density-32 and density-80 PMap result markers match the pinned oracle. The full gate transcript is retained at [phase4-gate.log](raw/phase4-pmap-export-002049/phase4-gate.log).
