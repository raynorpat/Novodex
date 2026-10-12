# Fluid manager populated teardown (`phys_fn_003641`)

`nxFluidManagerDestroyArrays` switches the manager to its base vtable, calls the scalar deleting destructor for each non-null primary fluid, frees and clears both array headers, and conditionally tears down an initialized extension. The registered `NxPhysicsSimulationTests` fixture seeds two fake fluid objects plus allocator-owned primary and secondary arrays, and invokes the manager deleting destructor with `deleteObject=0`.

The restored oracle/candidate pair reports `simulation fluid manager-populated-destructor calls=2 flags=1 target=1 arrays-cleared=1` with exact staged output. A row-specific mutation that suppresses the per-fluid deleting-dispatch call is detected by Phase 7: the candidate reports zero calls, exits 1, and changes the staged stdout by 54 bytes. After restoring the call, the full Phase 7 gate passes at 1,698 evaluated assertions against the 1,697 floor.
