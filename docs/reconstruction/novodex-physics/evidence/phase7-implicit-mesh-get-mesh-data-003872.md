# Phase 7: implicit-mesh `getMeshData` (`phys_fn_003872`)

IDA at RVA `0x0008d1f0` shows the public `NpImplicitMesh::getMeshData` body: acquire the read lock through `[this+0x10]`, obtain the internal record's `+8` data member through the helper at `0x0008ac40`, copy twelve dwords into the hidden `NxMeshData` return buffer, release the lock, and return that buffer. The CMake Release map resolves the candidate implementation to `?getMeshData@NpImplicitMesh@@UAE?AVNxMeshData@@XZ` at preferred address `0x1002e1c0` in `ImplicitMesh.obj`.

The registered `NxPhysicsFluidEmitterAbiTests` probe calls the pinned oracle RVA and the map-resolved candidate DLL method on three independently seeded records. It compares all 48 result bytes against the expected record contents, the hidden return-buffer identity, lock release and owner state, and x86 stack balance. Restored control: `cases=3 matched=3 balanced_stacks=3 released_locks=3 mismatches=0`.

Mutation validation changed the candidate copy source from record `+8` to `+12` in a temporary Release build. The oracle remained exact while the candidate failed all three cases (`matched=0`, `mismatches=3`; test exit 1). Restoring the source and rebuilding returned to the exact result above. Logs are retained in `evidence/raw/phase7-implicit-mesh-get-mesh-data-003872-20261017/`.

This closes the tested getter row and records its candidate map correspondence. It does not close adjacent implicit-mesh methods or the remaining FluidModel implementation.
