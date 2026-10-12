# Fluid collision sphere-array appender (`phys_fn_003543`)

`phys_fn_003543` at RVA `0x87740` appends one 84-byte sphere record to the caller-owned typed array. The private candidate implementation is `nxFluidCollisionAppendSphere` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA shows the helper checks shape type at `+0xd0`. For type 1 it stores the sphere geometry word from `+0xe0`, packs the collision record after that word, copies 84 bytes to `array.data + 84 * array.count`, increments the count, and returns the descriptor. Other types return the shape unchanged without touching the array.

`NxPhysicsFluidEmitterAbiTests` compares the pinned oracle RVA with the candidate DLL for a selected sphere and an ignored non-sphere. The selected fixture gives `shape+0xe0` and `shape+0xe4` distinct values, compares the defined 80-byte record prefix and untouched tail, and checks count, return value, and cdecl stack balance. The restored control reports `candidate_present=1`, `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation changed the selected geometry source from `shape+0xe0` to `shape+0xe4`. The differential rejected it (`oracle_contract=0`, `mismatches=1`, process exit 1); restoring the committed source returned the differential to zero mismatches. The passing candidate DLL SHA-256 is `8245ceb04f5c057d014346cff77dfbbb15a9d84200678f599356a43c00884d45`; the pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

This closes only the sphere typed-array appender. The plane, capsule, box, and mesh appenders, parallel scene collector `phys_fn_003555`, and enclosing dirty static-collision builder `phys_fn_003505` remain open.

Fresh Release Phase 5 and Phase 7 gates both pass with the committed candidate: Phase 5 at 2,654/2,654 registered assertions and Phase 7 at 1,681/1,681. The Phase 5 gate also re-exercises all five family-specific collision-object secondary-destructor probes and primary deleting-destructor probes.
