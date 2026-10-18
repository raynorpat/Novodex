# Fluid surface-mesh owner binding teardown (`phys_fn_003687`)

The fluid-surface manager deleting destructor clears its SDK pointer binding before destroying the public mesh. This matches the pinned oracle destructor at RVA `0x0008af20` (93 bytes): install the deleting vtable, invoke the backend destroy callback, destroy and clear the public mesh at `this+0x38`, remove the manager name entry, clear the SDK binding, run the base destructor, and conditionally free when flag bit 0 is set. The existing reconstruction already performed every other teardown step but omitted the binding clear.

The fix is in `Physics/src/Scene.cpp`, in `nxFluidSurfaceMeshDeletingDestructor`: `nxSetSdkPointerBinding(manager, 0)` now follows `nxImplicitMeshSetName(manager, 0)`. The public headers are unchanged.

`NxPhysicsSimulationTests` seeds a sentinel binding on the live licensed surface-mesh manager, verifies it is present, calls the public `releaseSurfaceMesh`, and then verifies that lookup returns null. The row marker is `simulation implicit-mesh owner-binding-clear row=003687 set=1 before=1 cleared=1 matched=1`.

The mutation removed only the binding clear. Oracle reported `set=1 before=1 cleared=1 matched=1`; candidate reported `set=1 before=1 cleared=0 matched=0`, failed with exit 1, and produced `stdout_delta=74`. Restored source and candidate reported identical output (`stdout_delta=0`, exact stderr).

Fresh gates passed: Phase 5 `2,911/2,911`, Phase 6 `1,767/1,767`, and Phase 7 `3,050/3,050` coverage assertions. Candidate map symbol: `?nxFluidSurfaceMeshDeletingDestructor@@YIPAXPAX0E@Z` at RVA `0x0005c0e0`.

Raw logs and their hashes are in `evidence/raw/phase7-fluid-surface-owner-binding-clear-003687-20261018/`.
