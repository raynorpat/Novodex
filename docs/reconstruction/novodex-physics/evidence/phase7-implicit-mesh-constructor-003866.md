# `NpImplicitMesh` constructor (`phys_fn_003866`)

IDA identifies `phys_fn_003866` at RVA `0x0008d0c0` as the constructor for the private `NpImplicitMesh` wrapper. The constructor initializes the primary `NxImplicitMesh` vptr, constructs `NpImplicitMeshLockBase` at `this+8` (secondary vptr and two null lock links), stores the supplied internal record at `this+0x14`, installs the final primary vptr, and returns `this`. The existing implementation is in `Physics/src/ImplicitMesh.cpp`; the Release map resolves `??0NpImplicitMesh@@QAE@PAX@Z` in `ImplicitMesh.obj`. No public SDK header changed.

The registered `NxPhysicsFluidEmitterAbiTests` differential calls the pinned oracle RVA and the candidate map symbol on separate 0xcd-initialized 0x18-byte objects. It checks the primary and secondary vptrs against each module's tables, both lock links, the exact per-side record pointer, returned object identity, and x86 stack balance. Restored control reports:

```text
implicit mesh constructor row=003866 primary=1 secondary=1 links=1 record=1 return=1 stack=1 mismatches=0
```

For mutation falsification, the candidate-only constructor initializer was changed from `mInternalRecord(internalRecord)` to `mInternalRecord(0)`, then the CMake Release DLL and ABI test target were rebuilt. The oracle exits zero; the candidate exits one with `record=0, mismatches=1`. The initializer was restored, the candidate rebuilt, and the oracle/candidate transcript returned to exact equality (`stdout_delta=0`, exact stderr). The oracle DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; the restored candidate DLL SHA-256 is recorded in the raw manifest.

Full Phase 5 and Phase 7 gate results and archived raw transcripts are recorded under `evidence/raw/phase7-implicit-mesh-constructor-003866-20261017/`.
