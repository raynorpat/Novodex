# Public implicit-mesh blend setter — row 003886

IDA identifies `phys_fn_003886` at RVA `0x8d4d0` as `NxImplicitMesh::setBlend`. It reports the unavailable-feature warning (code 206, `NpImplicitMesh.cpp:65`), then takes the scene write lock, checks callback re-entry, dispatches the value through the private mesh record, and releases the lock. The source implementation is `NpImplicitMesh::setBlend` in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh fixture now has a row-specific assertion for the warning and backend call:

```text
simulation implicit-mesh set-blend row=003886 matched=1 value=00000000 calls=1 errors=1 code=206 line=65
```

Removing the backend setter is mutation-rejected: the candidate still reports the unsupported warning but has zero setter callbacks and `matched=0` (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=68`). After restoring and rebuilding `NxPhysics.dll`, the oracle and candidate are exact (`stdout_delta=0`, exact stderr).

The row is registered in the Phase 5/6/7 simulation gates. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-set-blend-003886-20261016/`](raw/phase7-implicit-mesh-set-blend-003886-20261016/).
