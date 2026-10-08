# MeshShape constructor mutation — `phys_fn_001379`

Date: 2026-10-08

`MeshShape::MeshShape` at RVA `0x00027db0` initializes the mesh words at offsets `+0xe0` and `+0xe4` before installing its collision-object back-pointer and mesh shape sentinel. The registered `NxPhysicsObjectLayoutTests` poisoned-buffer differential observes both words, the sentinel, and the collision-object owner pointer.

Changing the `+0xe4` initialization from zero to `0xdeadbeef` is detected: the mesh candidate row reports `ok=0` and the aggregate layout differential reports two mismatches. After restoring the zero initialization, the candidate reports `word_e0=00000000`, `word_e4=00000000`, `colobj_ok=1`, digest `422a1f78`, and the complete layout differential ends with zero mismatches.

The fresh Win32 Release Phase 5 gate passes all 19 staged targets and 2,565/2,565 registered coverage assertions (`build/phase5-mesh-ctor-final.log`). No public Physics headers changed.

Logs: `build/phase5-mesh-ctor-mutation.log` and `build/phase5-mesh-ctor-restored.log`.
