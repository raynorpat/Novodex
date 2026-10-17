# Phase 7: implicit-mesh deleting destructor (`phys_fn_003868`)

IDA identifies RVA `0x0008d100` as the `NpImplicitMesh` scalar deleting destructor. It transitions the primary vptr to the `NxImplicitMesh` base, installs the secondary `NpImplicitMeshLockBase` vptr while running that base destructor, preserves the internal-record pointer, and conditionally frees the complete object when the deleting flag is set. The existing `NpImplicitMesh` destructor and multiple-inheritance definition let MSVC emit this ABI path; no public header changes are needed.

The registered `NxPhysicsFluidEmitterAbiTests` probe calls the pinned oracle function and the candidate map-resolved deleting destructor. For flag zero it compares primary/secondary vptr ownership, cleared lock links, preserved internal-record pointer, return identity, and x86 stack balance. For flag one it allocates scratch objects through each DLL's own scalar `operator new`, invokes its deleting destructor, and checks returned pointer identity. Restored output: `primary=1 secondary=1 links=1 record=1 return=1 stack=1 flag1_free=1 mismatches=0`.

Mutation validation cleared `mInternalRecord` from `NpImplicitMesh::~NpImplicitMesh` in a temporary Release build. The oracle/candidate probe rejected it (`record=0`, `mismatches=1`; oracle exit 0, candidate exit 1). Restored source was rebuilt and the direct staged-pair differential passed.

The gate markers are registered in Phase 5 and Phase 7. The corresponding complete phase-gate logs and restored/mutant differential logs are archived in `evidence/raw/phase7-implicit-mesh-deleting-dtor-003868-20261016/`.
