# Current-main standalone and DemoGame checkpoint — 2026-10-16

Source revision: `585f11f8f3786945ea2b60adc50879d442871095`.

The current committed layout-test source and its expected oracle digest are
already aligned: `NxPhysicsObjectLayoutTests` reports `layout oracle
digest=3acf0dc7`. The earlier uncommitted fixture edit from the October 8
worktree changes the same unusual-float inputs, but its `5943e78a` digest was
for an older source snapshot. It is superseded by the current-main test and
baseline; no stale edit or baseline was transplanted.

Fresh Win32 Release verification used
`D:\github\Novodex\build\phase5-main-validation-20261016`:

- Phase 5 passed at 2,825/2,825 assertions.
- Phase 7 passed at 2,958/2,958 assertions.
- In both gates, `NxPhysicsSimulationTests` completed on oracle and candidate
  with exit 0, `stdout_delta=0`, and exact stderr.
- The simulation transcript includes the 1,000-step soak and settled stack,
  repeated SDK/Scene lifecycle, broadphase selectors 0/1/2 with bounded and
  unbounded cases, pair ordering and coherent-cache movement, nonblocking
  result states, kinematic state, trigger enter/stay/leave, and deferred
  contact-report delivery.
- Release CTest ran the 48 registered `^Viewer` selections. All 43 executed
  tests passed, including all available scene demos and both
  `ViewerPhysicsStep` and `ViewerPhysicsContact`; the five pinned-oracle cases
  (CowPile, PMapTest 8/10/12, and TruckDemo) were skipped by their existing
  exact-signature rules.

Candidate module hashes:

| DLL | SHA-256 |
|---|---|
| `NxPhysics.dll` | `dbfec8a73922b8819a47a7f6a201f017661ec4dbd2e14138b52a2c408856ec1a` |
| `NxFoundation.dll` | `1255221745c1b8e84d2a033bf7b3ed5de16ade5b4f5762a0bffa057c7d4994f1` |

Raw run logs are in the ignored build directory: `phase5-main-validation-20261016.log`,
`phase7-main-validation-20261016.log`, and `viewer-scenes-main-final-20261016.log`.
This satisfies the roadmap's T1 standalone-test entry criteria for its listed
simulation fixtures. It does not complete the broader joint-family or
full-DLL reconstruction work.

The next staged `DemoGame PhysTest` attempt could not establish the Unreal
baseline in this run environment. The oracle process loaded the map and
reached level setup, then failed in `UD3DRenderDevice::InitDevice` with
`D3DERR_NOTAVAILABLE` before `Initializing Engine Completed`; the modal error
left the process running until the test timeout. The candidate was not run, so
this is not a candidate differential result. The installed oracle DLL hashes
remained `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
and `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`.
No installed DLL was replaced. T2 remains pending a successful interactive
D3D launch; further reconstruction can continue independently.
