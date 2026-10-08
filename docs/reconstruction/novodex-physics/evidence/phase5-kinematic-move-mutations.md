# Phase 5 kinematic move mutation closures

Date: 2026-10-07

The existing `NxPhysicsActorDynamicsTests` staged-pair target exercises the
public kinematic `moveGlobalPosition`, `moveGlobalPose`, and
`moveGlobalOrientation` methods. Its clean oracle/candidate transcript matched
before the mutations (`stdout_delta=0`, both exits 0, exact stderr), and the
restored Release DLL also matched exactly after all mutation runs. The restored
candidate DLL SHA-256 is
`0e7784fe6b8b7526f6e1e697df0e070c530e605a72bb262ea752dc83c68f51e4`.

Each mutation changed one row's computed target by reversing one translation
sum or offsetting its X component by one unit before storage. Both processes still
exited normally with exact stderr, and each mutation changed registered public
target-state observations:

| Row | Mutation | Candidate DLL SHA-256 | Detection |
| --- | --- | --- | --- |
| `phys_fn_000090` | `moveGlobalPosition`: subtract input Z from the local mass Z instead of adding it | `fec2a53f51807cd14a8ecafa5a316ffa38462746c36c3392f57ac1b6b85042b7` | `stdout_delta=16` |
| `phys_fn_000124` | `moveGlobalPose`: add `1.0f` to the computed target X before storage | `7702ced6b7191efecf392ee066965eb4ca486636280e335134d3b55d16b52e20` | `stdout_delta=14` |
| `phys_fn_000126` | `moveGlobalOrientation`: add `1.0f` to the computed target X before storage | `5d7e0f4f4900130b25b716fd184b1fec8253b53a49e9b7e726aaa05e5d66da29` | `stdout_delta=16` |

The clean baseline and each mutant used the staged `NxPhysicsActorDynamicsTests`
pair runner. The first attempt to test the position mutation exposed that
building only the executable does not necessarily rebuild the staged DLL; its
unchanged module hash identified the stale artifact. The mutation was then
rebuilt explicitly through the `NxPhysics` target before testing. All recorded
mutant hashes above are from those rebuilt staged DLLs.

The three mutations are caught by already registered lines in
`docs/reconstruction/novodex-physics/tools/gate_targets.ps1`; no coverage floor
increase was needed. This closes the tested kinematic movement rows, not the
full actor-vtable or Phase 5 object audit.
