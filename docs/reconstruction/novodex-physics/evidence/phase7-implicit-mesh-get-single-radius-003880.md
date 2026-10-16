# Public implicit-mesh single-radius getter — row 003880

IDA identifies `phys_fn_003880` at RVA `0x8d3c0` as the public `NxImplicitMesh::getSingleRadius` path. It enters the Scene read lock through the mesh's `+0x10` link, reads the internal record at `+0x14`, calls the backend single-radius getter, releases the read lock, and returns the float. The source implementation is `NpImplicitMesh::getSingleRadius` in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh simulation fixture observes the initial value, sets a new value, and observes it again. A row-specific assertion checks both float bit patterns and the two backend getter calls:

```text
simulation implicit-mesh get-single-radius row=003880 matched=1 initial=3c23d70a updated=3c449ba6 calls=2
```

The mutation replaces the getter result with `0.0f` while retaining the read-lock bracket. It is rejected: the candidate reports zero values and calls, exits 1, and differs from the oracle by 68 bytes. After restoring and rebuilding `NxPhysics.dll`, both oracle and candidate exit 0 with `stdout_delta=0` and exact stderr.

The row is registered in the Phase 5/6/7 simulation gates, which pass at 2,817/2,817, 1,706/1,706, and 2,947/2,947 assertions. The reconstruction-tool suite passes all 816 tests. Gate transcripts and mutation/control evidence are in [`evidence/raw/phase7-implicit-mesh-get-single-radius-003880-20261016/`](raw/phase7-implicit-mesh-get-single-radius-003880-20261016/).
