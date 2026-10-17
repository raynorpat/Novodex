# Phase 7 row 000499: `NxPlaneShapeDesc::setToDefault`

`phys_fn_000499` at oracle RVA `0x0000fec0` initializes the common shape descriptor and the ground plane: normal `(0,1,0)` at `+0x4c..+0x57` and distance `0` at `+0x58`. The unchanged public implementation is in [NxPlaneShapeDesc.h](../../../../Physics/include/NxPlaneShapeDesc.h).

The current CMake Win32 Release map resolves the emitted candidate method `?setToDefault@NxPlaneShapeDesc@@UAEXXZ` at VA `0x1006e3b0` (RVA `0x0006e3b0`) in `Scene.obj`. Its inline definition is in the unchanged public header `Physics/include/NxPlaneShapeDesc.h`. `NxPhysicsObjectLayoutTests` resolves this entry through `NxPhysics.map`, invokes the actual loaded candidate DLL method on poisoned storage, and compares bytes `+8..+0x5b` with the oracle call. Control output: `tmplinit row=phys_fn_000499 candidate_dll_vs_oracle bytes=84 mismatches=0` (exit 0).

The implementation-level mutation patches the first byte of the loaded candidate method to `ret`. The fixture then reports `mismatches=80` and exits 1. The four-byte name field at `+0x48` is intentionally untouched by both methods and therefore retains poison; the mutation is rejected by the other initialized bytes. The restored control reports zero mismatches. The candidate DLL SHA-256 is `b9c0ae2b31ed23416c3ba9550f6bd07b31182f11ba3f46e55ab04b0a84433629`; the fixture SHA-256 is `3575827337ec861ac6dc21061278de7da01afb424c50e7dd129277dbc7806a67`. No public Physics headers changed.

This closes the row as `oracle_differential_falsified`. The Phase 7 total increases to 337 closed and 223 deferred rows; full DLL reconstruction remains open.
