# Fluid collision scene shape collection (`phys_fn_003549`)

`phys_fn_003549` at RVA `0x87880` enumerates `Scene::shapes` at scene offset `+0x624` and routes leaves to `phys_fn_003511`. Its private candidate implementation is `nxFluidCollisionCountSceneShapes` in `Physics/src/FluidCollision.cpp`.

IDA confirms the container is read through its wrapper at `scene+0x624`: the backing collection pointer is at wrapper `+0x1c`; the collection start slot and active run lengths are at backing `+0x04`, `+0x08`, and `+0x0c`; entries point to nodes whose shape is at `node+4`. A shape of type 5 is a compound; the helper walks the pointer range `[shape+0xe0, shape+0xe4)` and classifies each member instead of the compound.

`NxPhysicsFluidEmitterAbiTests` directly compares the pinned oracle and candidate DLL for an empty scene, a mixed four-shape scene with a two-member compound, and a dynamic scene with a three-member compound that includes an owner-excluded member and a refresh-marked member. Across nine routed leaves it checks literal per-type counter expectations, collection return counts `0/4/2`, complete builder state, and cdecl stack balance. The restored differential reports `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation skipped compound-member classification. Both nonempty cases detected missing counters (`counts=1/0`, `mismatches=2`); restoring the calls returned the direct differential to zero. The mainline candidate used for the passing Phase 5 and Phase 7 gates has SHA-256 `b91e40c51c76cb01dc6ec849c57976c786971054d6ea422ffa3c1f336a7e3c34`; the mutant build has SHA-256 `63d1e34852ad6eebe7fb7f9e36f8fa0ee56e4b4f9abc63c3fd5628fec147caf7`. The oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Phase 5 passes at 2,653/2,653 registered assertions and Phase 7 passes at 1,680/1,680. The parallel typed-array collector `phys_fn_003555` and the callback-backed static-collision builder `phys_fn_003505` remain open.
