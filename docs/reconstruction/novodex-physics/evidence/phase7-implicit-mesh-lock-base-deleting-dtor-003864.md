# Implicit-mesh lock-base deleting destructor (`phys_fn_003864`)

The IDA body at RVA `0x0008d0a0` is the scalar deleting destructor for `NpImplicitMeshLockBase`. It installs the standalone base vptr, preserves both lock links, conditionally releases the object for delete flag bit zero, returns `this`, and uses `ret 4`. CMake/MSVC emits the candidate scalar deleting destructor from the existing private base class.

The registered `NxPhysicsFluidEmitterAbiTests` differential invokes oracle RVA `0x0008d0a0` and the candidate map symbol `??_GNpImplicitMeshLockBase@@UAEPAXI@Z`. On flag zero it checks oracle/candidate vptrs, seeded write/read link preservation, return identity, and x86 stack balance. On flag one it allocates independent scratch objects through each DLL and observes their frees. Restored control reports `mismatches=0`.

A source mutation that cleared `mWriteLockLink` was rejected: the marker changed to `oracle_links=1 candidate_links=0 mismatches=1`, and the candidate process exited 1. The restored candidate DLL reports `oracle_links=1 candidate_links=1` and exits 0. Fresh Phase 5/6/7 gates pass at 2,836/2,836, 1,720/1,720, and 2,969/2,969 coverage assertions; the raw transcripts are archived.
