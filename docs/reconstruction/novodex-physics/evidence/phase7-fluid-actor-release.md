# Phase 7 fluid actor-release notification

The actor-release chain now walks seeded FluidManager and NpFluid arrays. It
preserves the exact unavailable-backend warning, then visits each internal
emitter whose frame actor equals the released body, sets broken-actor-reference
bit 2, captures the emitter's global pose, clears its frame actor, writes the
pose back as the local/global pose, and clears the backend attached flag.

`NxPhysicsFluidEmitterAbiTests` calls oracle `phys_fn_003635` at RVA
`0x00089ce0` and the candidate manager notification helper resolved from the
candidate DLL map. Its fixture contains one fluid and three emitters, with
matches at indices 0 and 2 and a nonmatching body at index 1. It verifies the
preserved flag bits, both actor-pointer transitions, untouched nonmatch state,
12 pose words, backend arguments, callback order, and callback payloads.
Restored control: `oracle_callbacks=6`, `candidate_callbacks=6`,
`state_mismatches=0`, `callback_mismatches=0`.

Three source mutations were caught before restoring the exact control:

- `phys_fn_003485`: reverse the frame-actor comparison. The direct oracle
  differential rejects the mutant with `mismatches=49`.
- The zero-detach path of `phys_fn_003622`: report attached=1 instead of 0 to
  `EmitterSetAttachedFlag`. The differential rejects it with
  `mismatches=4`.
- `phys_fn_003635`: skip the manager's fluid loop. The differential rejects
  it with `mismatches=33`.

`NxPhysicsSimulationTests` compares the public actor-release path with an
empty disabled manager. Suppressing its conditional manager notification call
is caught with `stdout_delta=2`; after restoration, both pair runs exit zero
with `stdout_delta=0` and exact stderr. The seeded direct-row fixture exercises
live fluid/emitter contents independently because the public
`NxScene::createFluid` backend path remains unavailable. Together these checks
cover the `phys_fn_000628` call site and the reachable release chain.

`phys_fn_003622` (`NpFluidEmitter::setFrameActor`) now reconstructs both its
detach and attach/update arms. The direct oracle/candidate differential checks
the zero-detach behavior plus nonzero actor attachment through both supported
pose sources: a rotated matrix-backed frame actor and a dynamic-body
quaternion frame with all four components nonzero.
The tests compare the internal 12-word local pose, backend world-pose callback
payload, attached flag, callback order, and return value. Restored control:
both attach cases report `mismatches=0`. Forcing the nonzero attached-flag
argument to zero is caught with `mismatches=2` across the two cases.
The fresh full Phase 7 gate passes at 1,652/1,649 registered coverage
assertions.
