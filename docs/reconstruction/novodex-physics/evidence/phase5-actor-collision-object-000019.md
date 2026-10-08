# Actor shape collision-object helper (`phys_fn_000019`)

The row at oracle RVA `0x00001540` reads the actor body's root shape. It returns null when the body has no shape, the mesh collision object at `shape+0xf0` for a kind-5 mesh, and `shape+0x9c` for other shapes. The private source implementation is `nxBodyCollisionObject` in `Physics/src/ObjectModel.cpp`; the paired actor accessor is `nxActorCollisionObject`.

The pinned-oracle `NxPhysicsObjectLayoutTests` fixture drives the mesh, ordinary shape, and null-shape cases in its `actorsm` differential. In a targeted mutation, the kind-5 branch was changed to return `shape+0x9c`. The target caught it with `actorsm candidate ok=0 digest=81d5e7b9` and `layout candidate mismatches=1` (exit 1). Restoring the mesh pointer return produced `actorsm candidate ok=1 digest=1bdc2fa8`, `layout candidate mismatches=0`, and exit 0. The independent null-shape mutation also produced `actorsm candidate ok=0 digest=9caab999` and one mismatch; it was restored before the mesh-branch measurement.

The test executable links the private candidate helper from `ObjectModel.cpp` and invokes the pinned shipped DLL as the oracle. This is an oracle differential for the helper, not proof that a public consumer reaches this candidate helper through the rebuilt DLL. Final Phase 8 still requires DLL-route evidence where the row is reachable.
