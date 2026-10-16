# Public implicit-mesh single-radius setter — row 003882

IDA identifies `phys_fn_003882` at RVA `0x8d3f0` as the public `NxImplicitMesh::setSingleRadius` path. It takes the Scene write-lock link at `this+0x0c`, rejects lock failure, checks API callback re-entry, dispatches the requested value through the internal mesh record at `this+0x14`, clears re-entry, and releases the lock. The source implementation is `NpImplicitMesh::setSingleRadius` in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh fixture reads the initial single radius, sets a new value, and reads it back. Its row-specific line checks the two float bit patterns and exactly one backend setter call:

```text
simulation implicit-mesh set-single-radius row=003882 matched=1 initial=3c23d70a updated=3c449ba6 calls=1
```

The no-op mutation removes the backend setter. The candidate retains the initial value, reports zero setter calls and `matched=0`, exits 1, and differs from the oracle by 70 bytes. After restoring and rebuilding `NxPhysics.dll`, both oracle and candidate exit 0 with `stdout_delta=0` and exact stderr.

The row is registered in the Phase 5/6/7 simulation gates. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-set-single-radius-003882-20261016/`](raw/phase7-implicit-mesh-set-single-radius-003882-20261016/).
