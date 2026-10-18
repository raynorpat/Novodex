# Capsule and sphere pose-setter lock behavior

On 2026-10-18, the registered `NxPhysicsActorShapeMutationTests` staged-pair differential was extended to exercise all six capsule and all six sphere public pose setters while another thread owns the Scene write lock. The pinned DLL reports the shape-family source file and line for every rejected operation and leaves the pose unchanged. Before the reconstruction, these twelve vtable slots inherited the box handlers and produced the wrong diagnostic source.

`Physics/src/NpActor.cpp` now installs capsule- and sphere-specific handlers in slots 7, 8, 9, 16, 17, and 18. Each handler follows the same refusal and success path as the oracle; only the family-specific diagnostic differs. The public header set is unchanged.

The test first failed against the old handlers (`stdout_delta=24`). With the reconstruction restored, oracle and candidate both exited zero, with `stdout_delta=0` and exact stderr. The transcript includes the expected capsule diagnostic at `NpCapsuleShape.cpp:0x10`, the sphere diagnostic at `NpSphereShape.cpp:0x16`, unchanged locked poses, and six reports per family.

Each of the twelve rows was then mutated independently by routing its slot back to the box handler. All twelve mutations were detected (`stdout_delta=2` each). Every case was rebuilt, differentially run, restored, and rebuilt before proceeding to the next. Raw red/green and per-row build, mutation, and restored-build logs are in `evidence/raw/phase3-capsule-sphere-setter-locks-001087-001179-20261018/`.

The Phase 3 ledger now records 107 closed rows and 285 deferred rows. These rows are discharged by Phase 5's public setter differential; their individual closure entries and candidate map references are recorded in the inventory. Phase 3's coverage floor remains 984; Phase 5/6/7 coverage floors each increase by three assertions.

Fresh Phase 3, 5, and 6 gates passed at 984, 2,899, and 1,756 assertions. The first Phase 7 run had a one-time access violation in the oracle `NxPhysicsSimulationTests` process; the isolated staged-pair rerun matched exactly, and a full Phase 7 retry passed at 3,038 assertions. Both gate transcripts are retained in the raw evidence directory.
