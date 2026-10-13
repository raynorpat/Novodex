# Actor-group pair-flag SDK wrappers: rows 000269 and 000271

`NxPhysicsSDKTests` now creates two live scenes before it sets, reads in both
key orders, and clears actor-group pair flags. This drives the complete
`NpPhysicsSDK` wrapper loops: row `phys_fn_000269` acquires both scene write
locks before forwarding the setter, and row `phys_fn_000271` acquires both
scene read locks before forwarding the getter. The required marker
`step=actor_group_pair_flags_scenes count=2` guards the new lock-bearing setup;
the existing round-trip marker guards the values.

Each wrapper was independently replaced with a no-op and rebuilt. The staged
oracle differential rejected row `phys_fn_000269` with `stdout_delta=6` and
row `phys_fn_000271` with `stdout_delta=2` (both processes exited zero and
stderr matched). Restoring each wrapper produced an exact transcript
(`stdout_delta=0`, `stderr_exact=True`).

Raw logs: `evidence/raw/phase2-actor-group-pair-flags-000269-000271/`.

The fresh Phase 2 gate passes at 7/7 assertions. Its SDK differential remains
exact. The additional Phase 3 registration adds one assertion there, and the
fresh Phase 3 gate passes at 561/561. That run exposed a link defect in
`NxPhysicsCollisionTests`: its standalone source list compiled `Scene.cpp`
without the fluid implementation files already present in
`NxPhysicsInternalTests`. Adding `FluidCollision.cpp` and `NpFluidEmitter.cpp`
restored the target's seven missing definitions and the Phase 3 build and gate
now pass.
