# Public implicit-mesh triangle-size setter — row 003890

IDA identifies `phys_fn_003890` at RVA `0x8d5d0` as `NxImplicitMesh::setTriangleSize`. It takes the scene write lock, checks callback re-entry, dispatches the value through the internal mesh record, and releases the lock. The source implementation is `NpImplicitMesh::setTriangleSize` in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh fixture verifies the initial and updated values, one backend setter callback, and valid callback arguments:

```text
simulation implicit-mesh set-triangle-size row=003890 matched=1 initial=3ba3d70a updated=3bc49ba6 calls=1 callback-errors=0
```

Removing the backend setter is mutation-rejected: the candidate retains the initial value, reports zero setter callbacks, and has `matched=0` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=68`). After restoring and rebuilding `NxPhysics.dll`, the oracle and candidate are exact (`stdout_delta=0`, exact stderr).

The row is registered in the Phase 5/6/7 simulation gates. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-set-triangle-size-003890-20261016/`](raw/phase7-implicit-mesh-set-triangle-size-003890-20261016/).

The first Phase 6 invocation also recorded an unrelated allocator-reuse mismatch at row `phys_fn_004810`. Five direct reruns of that probe and the fresh Phase 6 gate passed; the initial transcript is retained with the evidence bundle.
