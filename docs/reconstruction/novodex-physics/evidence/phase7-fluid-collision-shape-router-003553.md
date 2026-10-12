# Fluid collision shape-array router (`phys_fn_003553`)

`phys_fn_003553` at RVA `0x87a40` selects a typed collision array and dispatches a shape to its appender. The private candidate implementation is `nxFluidCollisionShapeArrayRouter` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA confirms the cdecl `(builder, shape)` signature and its early-return order: shape flag `0x200` returns that bit; an owner whose `+0x14` flags contain `4` returns the owner; otherwise the shape type and flag `0x80` choose the destination array. Without `0x80`, types 0-4 route to plane/sphere/box/capsule/mesh descriptors at offsets `0x30/0x48/0x3c/0x54/0x00`. With `0x80`, types 0-3 route to offsets `0x6c/0x78/0x90/0x84`; other types return the type unchanged.

`NxPhysicsFluidEmitterAbiTests` covers 13 cases: all five ordinary routes, the four `0x80` routes, unsupported type 4 under `0x80`, both early returns, and an invalid type. It checks the selected descriptor and record, untouched nonselected arrays, return values, counts, and stack balance. The oracle-only run passed its fixture contract while red because the candidate symbol was absent; the restored direct differential reports `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation routed ordinary box shapes to descriptor `builder+0x30` instead of `builder+0x3c`. The type-2 case detected the wrong return and destination (`mismatches=1`, exit 1); restoring `+0x3c` returned the 13-case fixture and fluid-emitter aggregate to zero mismatches. Passing candidate SHA-256 `20cf41661aa185fbe24ed8478d260a00921b75a85b8daea5026d90a14cdfa2cc`; pinned oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw oracle-only, passing differential, and mutation transcripts are in `evidence/raw/phase7-fluid-collision-shape-router-003553/`. This closes only the router. Scene collector `phys_fn_003555` and dirty static-collision builder `phys_fn_003505` remain open.

Fresh mainline Release verification on 2026-10-12 passes Phase 5 at 2,659/2,659
and Phase 7 at 1,686/1,686 assertions. The 810 reconstruction-tooling tests,
inventory validation, and the 80-file public Physics header manifest also pass.
The passing candidate DLL hash is recorded above.
