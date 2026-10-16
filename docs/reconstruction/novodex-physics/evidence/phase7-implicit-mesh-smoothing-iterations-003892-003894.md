# Public implicit-mesh smoothing-iteration accessors — rows 003892 / 003894

IDA identifies `phys_fn_003892` at RVA `0x8d680` as `NxImplicitMesh::getSmoothingIterations`: it brackets the private record getter with the scene read lock. `phys_fn_003894` at RVA `0x8d6b0` is `NxImplicitMesh::setSmoothingIterations`: it uses the scene write lock and callback-reentry guard before dispatching through the private record. The source implementations are in `Physics/src/ImplicitMesh.cpp`.

The licensed surface-mesh fixture now has separate row assertions:

```text
simulation implicit-mesh get-smoothing-iterations row=003892 matched=1 initial=6 updated=8 calls=2
simulation implicit-mesh set-smoothing-iterations row=003894 matched=1 value=8 calls=1 callback-errors=0
```

Independent mutations falsify both rows. Replacing the getter result with zero is rejected (`candidate_exit=1`, `stdout_delta=70`); removing the setter leaves the initial value and is rejected (`candidate_exit=1`, `stdout_delta=70`). The restored oracle/candidate differential passes with `stdout_delta=0` and exact stderr.

The rows are registered in the Phase 5/6/7 simulation gates. Gate transcripts and both mutation/control records are in [`evidence/raw/phase7-implicit-mesh-smoothing-iterations-003892-003894-20261016/`](raw/phase7-implicit-mesh-smoothing-iterations-003892-003894-20261016/).
