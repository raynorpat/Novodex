# NxFluidEmitter base deleting destructor (`phys_fn_003790`)

IDA identifies RVA `0x8c2b0` as the NxFluidEmitter base scalar deleting destructor: it installs the base vptr and frees the 8-byte object only when deleting flag bit 0 is set. CMake emits the candidate implementation as `??_ENxFluidEmitter@@MAEPAXI@Z` at `0x1003cea0` in `NpFluidEmitter.obj`; the public header definition was not changed.

The registered `NxPhysicsFluidEmitterAbiTests` probe calls oracle and candidate directly with flags 0 and 1. It checks the base vptr, return identity, x86 stack balance, and a free through each DLL’s allocator. The restored control reports zero mismatches.

A candidate-only mutation changed the base-vptr immediate from `0x1012f304` to `0x1012f398`; the probe rejected it with `candidate_vptr=0` and `mismatches=1`. The candidate DLL was restored byte-for-byte. Fresh Phase 5 and Phase 7 gates pass at 2,839/2,839 and 2,972/2,972 assertions; the full reconstruction-tool suite passes 817 tests. Raw evidence is under `evidence/raw/phase7-fluid-base-deleting-dtor-003790-20261017/`.
