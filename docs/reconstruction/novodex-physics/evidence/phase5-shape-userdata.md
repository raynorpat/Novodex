# Runtime shape descriptor `userData` — 2026-10-06

`ASVehicle::PostInitRigidBody` discovers each vehicle wheel's ray shape by
matching `NxShape::userData` to the `NxWheelShape` pointer stored in the
capsule descriptor. DemoGame's `VehicleTest` exposed that the candidate created
the capsule but did not copy the descriptor pointer into the public shape
handle, leaving `vw->RayShape` null and tripping the engine assertion at
`UnSVehicle.cpp:190`.

Oracle `phys_fn_001347` (`ShapeBase::nxApplyDescriptor`, RVA `0x27740`) copies
the descriptor field at `+0x40` into the public handle at `+0x04`. The runtime
shape loader now applies that copy before dispatching the concrete family
loader, so the same behavior covers capsules and the mesh slot-12 path. No
public Physics headers were changed.

The `NxPhysicsSimulationTests` fixture assigns a marker to a capsule
descriptor and reads it back from `getShapes()[0]->userData`. Before the fix,
the pinned oracle printed `marker=1` and candidate printed `marker=0`; after
the fix both printed `simulation capsule-shape userdata actor=1 shapes=1
marker=1` and exited 0. The candidate was `NxPhysics.dll` SHA-256
`362cf33c64eee4d451bbc92e0841cda81f3ef0a84975c27669f0568d8f2c68c6`; the
oracle was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The rebuilt candidate then passed all three staged DemoGame maps — `Physics`,
`RayPhysTest`, and `VehicleTest` — with exit code 0. Each log reached map load,
engine initialization, and clean exit; `VehicleTest` no longer asserted on
`vw->RayShape`. The staged candidate loaded the same candidate DLL hash above.
The installed `D:\FlamingEnt__\Unreal_3\Binaries` DLLs were not modified.

The approved all-scenes Viewer sweep ran with
`ctest --test-dir build -C Release --output-on-failure -R '^ViewerSmokeScene_'`.
It exercised all 39 available scenes: 34 passed and five existing
oracle-signature cases skipped (`CowPile`, `PMapTest10`, `PMapTest12`,
`PMapTest8`, and `TruckDemo`).

The isolated fresh Phase 5 gate also passed: all 2,042 registered coverage
assertions evaluated, including the actor/layout/vtable oracle checks. The gate
verified the public-header manifest against both the engine and reconstruction
trees before building and running its differential suite.

Run artifacts are under `build/shape-userdata-{oracle,candidate}.log`,
`build/Testing/Temporary/LastTest.log`, and
`build/FluidGate/demo-game-smoke-20261006b/candidate/DemoGame/Logs/shapeuserdata-*.log`.
The isolated gate build and staged pairs are under
`build/phase5-userdata-gate` and `build/phase5-userdata-pairs`.
