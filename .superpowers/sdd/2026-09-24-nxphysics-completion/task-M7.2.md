# Task M7.2 — Fluid emitter post-step body state dispatch (`phys_fn_003624`)

## Goal

Reconstruct the caller at RVA `0x89930`, which first refreshes an emitter's world pose, then publishes its frame actor body's position, linear velocity, and angular velocity through FluidModel callbacks.

## Oracle contract

IDA decompilation shows `phys_fn_003624` calls `phys_fn_003605` unconditionally. If the internal emitter's frame-actor pointer at `this+0x14` is non-null, it reads the actor body at `actor+8`, copies body-state vectors at offsets `+0x158`, `+0x34`, and `+0x40`, then invokes `EmitterSetBodyPosition`, `EmitterSetBodyLinearVelocity`, and `EmitterSetBodyAngularVelocity` in that order using the fluid/backend/emitter handles.

## Falsifiable test

Call the pinned oracle at `0x89930` and a map-resolved candidate helper using parallel emitter/fluid/backend/actor/body layouts. Cover the no-actor branch and an actor with an attached body populated with asymmetric non-binary state vectors. Compare callback order, all arguments, vector words, return values, and x86 stack balance. A vector-offset or dispatch-order mutation must be rejected by the registered `NxPhysicsFluidEmitterAbiTests` target.

## Constraints

Do not edit public Physics headers. Keep caller closure separate from helper row `phys_fn_003605`; the extension-backed post-simulation lifecycle stays open until its manager path is directly tested.

## Gate and ledger

Register the new differential output with the Phase 7 gate and update the measured Phase 7 floor. Regenerate the completion backlog and work-unit map after inventory changes. Record exact row proof and tests in the shared M7 progress ledger.
