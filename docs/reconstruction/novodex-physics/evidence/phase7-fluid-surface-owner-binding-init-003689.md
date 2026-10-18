# Fluid surface-mesh SDK name binding (`phys_fn_003689`)

Oracle IDA decompilation identifies the 96-byte `__thiscall` routine at RVA `0x0008af80`. After the manager constructor and the descriptor validation in its caller, it copies the 0x30-byte mesh-data record, calls `ImplicitFluidMeshCreate` with backend handles and descriptor parameters, copies descriptor `userData` into the public mesh object, and stores the descriptor name pointer as the SDK pointer binding for the manager. It returns true.

The source path is `Physics/src/Scene.cpp#nxFluidInitializeSurfaceMesh`. It already implemented the descriptor copy, backend callback, public mesh `userData`, and the separate name lookup map, but omitted the SDK binding. It now calls `nxSetSdkPointerBinding(manager, const_cast<char*>(desc->name))`; the map resolves `nxFluidInitializeSurfaceMesh` to `Scene.obj` at RVA `0x00068c60`.

`NxPhysicsSimulationTests` creates a licensed fluid using `NxFluidDesc::surfaceMeshDesc`, retrieves the resulting manager, resolves the candidate/oracle SDK binding getter, and asserts the stored value equals the descriptor's name pointer. The omission mutation reproduces the original red result: oracle reports `resolved=1 manager=1 bound=1 matched=1`; candidate reports `bound=0 matched=0` and exits 1 (`stdout_delta=89`). Restored oracle and candidate both report `resolved=1 manager=1 bound=1 matched=1`, with `stdout_delta=0` and exact stderr.

Fresh gates passed: Phase 5 `2,912/2,912`, Phase 6 `1,768/1,768`, and Phase 7 `3,051/3,051` coverage assertions. Public headers are unchanged.

Raw mutation, restored, and gate logs plus compressed/uncompressed SHA-256 values are in `evidence/raw/phase7-fluid-surface-owner-binding-init-003689-20261018/`.
