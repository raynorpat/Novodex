# Public implicit-mesh surface radius setter — row 003878

IDA identifies `phys_fn_003878` at RVA `0x8d310` as the public `NxImplicitMesh::setSurfaceRadius` path. It takes the mesh's Scene write-lock link at `this+0x0c`, rejects lock failure, checks API callback re-entry, dispatches the requested value through the internal mesh record at `this+0x14`, clears the re-entry flag, and releases the lock. The source implementation is `NpImplicitMesh::setSurfaceRadius` in `Physics/src/ImplicitMesh.cpp`.

The licensed implicit-mesh fixture creates a surface mesh, observes its initial surface radius, sets it to `0.05f`, then checks the resulting value and backend callback count. A dedicated output line ties that assertion to this inventory row:

```text
simulation implicit-mesh set-surface-radius row=003878 matched=1 calls=1 reentry=1 errors=0
```

The no-op mutation removes the backend setter call. The candidate then reports `matched=0`, with zero setter and re-entry callbacks, and the staged differential rejects it (`candidate_exit=1`, `stdout_delta=66`). After restoring and rebuilding `NxPhysics.dll`, the oracle and candidate both exit 0 with `stdout_delta=0` and exact stderr.

The row is registered in the Phase 5/6/7 simulation gates, which pass at 2,816/2,816, 1,705/1,705, and 2,946/2,946 assertions. The reconstruction-tool suite passes all 816 tests. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-set-radius-003878-20261016/`](raw/phase7-implicit-mesh-set-radius-003878-20261016/).
