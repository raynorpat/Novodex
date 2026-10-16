# Public implicit-mesh name setter — row 003896

IDA identifies `phys_fn_003896` at RVA `0x8d760` as `NxImplicitMesh::setName`. It takes the scene write lock, checks callback re-entry, dispatches the name through the private mesh record, and releases the lock. The source implementation is `NpImplicitMesh::setName` in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh fixture now verifies both the initial name and the updated name after the setter:

```text
simulation implicit-mesh set-name row=003896 matched=1 initial=1 updated=1
```

Removing the backend name update is mutation-rejected: the candidate retains its initial name and reports `updated=0`, `matched=0` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=68`). After restoring and rebuilding `NxPhysics.dll`, the oracle and candidate are exact (`stdout_delta=0`, exact stderr).

The row is registered in the Phase 5/6/7 simulation gates. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-set-name-003896-20261016/`](raw/phase7-implicit-mesh-set-name-003896-20261016/).

Fresh Phase 5/6/7 gates pass at 2,823/2,823, 1,712/1,712, and 2,953/2,953 coverage assertions. One earlier Phase 6 attempt hit the object-layout allocator probe (`phys_fn_004807`/`004808`/`004810`); six isolated reruns and the subsequent full Phase 6 gate passed, so the transient transcript is retained alongside the passing gate log. The inventory validator reports zero unexplained records, and all 816 reconstruction-tool tests pass.
