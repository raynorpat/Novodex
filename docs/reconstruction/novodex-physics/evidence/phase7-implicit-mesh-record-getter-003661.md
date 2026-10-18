# Implicit-mesh data-record getter (`phys_fn_003661`)

IDA at RVA `0x0008ac40` identifies a no-argument `thiscall` getter that returns `this + 8`. The embedded object is `NxMeshData` in `NpImplicitMeshDataRecord`. Added `getMeshData()` to that private record and routed `NpImplicitMesh::getMeshData()` through it; no public header changed. The CMake Release map resolves `?getMeshData@NpImplicitMeshDataRecord@@QAEPAVNxMeshData@@XZ` in `ImplicitMesh.obj` at `0x1002e290`.

The registered `NxPhysicsSimulationTests` fixture calls the pinned oracle RVA and candidate map entry on three seeded records, checking the returned embedded-data pointer and x86 stack balance. A candidate `this + 12` mutant is rejected in all three cases (`mismatches=3`, `stdout_delta=4119`). The restored oracle/candidate differential is exact (`mismatches=0`, `stdout_delta=0`, `stderr_exact=True`).

Fresh Phase 5, 6, and 7 gates pass at 2,888/2,888, 1,745/1,745, and 3,027/3,027 assertions. The reconstruction tooling suite passes 836 tests and 743 subtests. Inventory validation reports 471 Phase 7 rows closed, 89 deferred, and zero unexplained census rows. Compressed transcripts, hashes, and gate logs are retained under [the raw evidence archive](raw/phase7-implicit-mesh-record-getter-003661-20261017/).
