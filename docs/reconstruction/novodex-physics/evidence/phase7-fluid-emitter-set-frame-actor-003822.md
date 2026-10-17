# Fluid emitter frame-actor setter (`phys_fn_003822`)

IDA at RVA `0x0008c900` shows `NpFluidEmitter::setFrameActor` acquiring the write lock, rejecting callback reentry, extracting the actor-body pointer from `NxActor+0x14`, binding that body through `phys_fn_003622`, refreshing the cached world pose through `phys_fn_003559`, then clearing the reentry flag and releasing the lock. The source wrapper now follows that sequence and reports the oracle diagnostics at lines 121 and 122.

The registered `NxPhysicsFluidEmitterAbiTests` fixture calls the pinned oracle, in-process candidate, and candidate DLL for attach and detach. It compares the four FluidModel callbacks in order, the stored actor-body pointer, all 0x60 bytes of local and cached pose state, and x86 stack balance. The restored differential reports `callback_checks=2`, `cache_checks=2`, `actor_checks=2`, `stack_checks=2`, and `mismatches=0`; both sides exit 0.

A mutation that passes the public `NxActor*` directly to the actor-binding helper instead of reading `NxActor+0x14` is rejected in the attach case (`actor_checks=1`, `mismatches=1`; oracle exit 0, candidate exit 1). Restoring the body-pointer extraction returns the differential to zero mismatches. The focused control and mutation logs are retained in [the raw evidence folder](raw/phase7-fluid-emitter-set-frame-actor-003822-20261017/).

Fresh Phase 5 and Phase 7 gates pass at 2,846/2,846 and 2,979/2,979 registered assertions. The reconstruction tooling suite passes with 836 tests and 743 subtests.

The row is registered in the Phase 5 and Phase 7 ABI target. Public SDK headers are unchanged.
