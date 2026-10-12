# Fluid collision shape counter (`phys_fn_003511`)

`phys_fn_003511` at RVA `0x86400` classifies a shape for the dirty static-collision builder. The private implementation is `nxFluidCollisionCountShapeType` in `Physics/src/FluidCollision.cpp`; the public headers remain unchanged.

IDA disassembly establishes a cdecl function with `(builder, shape)` passed on the stack. The shape carries the `0x200` refresh and `0x80` dynamic flags at `+0xde`, its owner pointer at `+4`, and its type at `+0xd0`. Owner byte `+0x14` bit 4 suppresses counting and returns the owner. Static types 0–4 increment builder offsets `+0x34`, `+0x4c`, `+0x40`, `+0x58`, and `+0x04`; dynamic types 0–3 increment `+0x70`, `+0x7c`, `+0x94`, and `+0x88`. Unsupported types return the type value; counted types return the builder; the refresh flag returns `0x200` before owner or type processing.

`NxPhysicsFluidEmitterAbiTests` compares the pinned oracle RVA with the candidate DLL for 13 cases: all nine supported type routes, both unsupported paths, owner exclusion, and refresh early return. It checks independent expected counter values, return semantics, complete builder and shape state, and cdecl stack balance. The restored differential reports `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation changed the dynamic type-2 counter offset from `+0x94` to `+0x90`. The fixture caught it in that case (`oracle count valid=1`, `candidate count valid=0`, `mismatches=1`). Restoring the source returned the differential to zero mismatches. The mainline candidate used for the passing Phase 5 and Phase 7 gates has SHA-256 `72490a915bef27d7db556b22096bb6dcc8f6e5c6644f8f81c5735041941db67a`; the mutant build has SHA-256 `8abee01c849091f6b95295c00398f7d967b8cf168665eb2e070b59d8cd645b81`. The oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Phase 5 passes at 2,652/2,652 registered assertions and Phase 7 passes at 1,679/1,679. The larger dirty static-collision builder `phys_fn_003505` remains open pending its collection/insertion helpers and callback-backed nonempty-mesh fixture.
