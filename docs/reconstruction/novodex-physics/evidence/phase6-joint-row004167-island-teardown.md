# Phase 6 row 004167: island-object teardown

`phys_fn_004167` is the island object's teardown routine called by `phys_fn_000760` when the object's pointer at body-record `+0x1e0` is non-null. The object is 0x1c bytes: pointer-array triples at `+0x00..+0x08` and `+0x10..+0x18`, with a preserved dword at `+0x0c`.

The implementation in `Physics/src/core/JointSupport.cpp` follows the oracle's order: release non-null elements from the second array first, then the first array; call vtable slot 0 with flag 1; free each non-null array begin pointer through the Foundation allocator; clear each triple; preserve `+0x0c`.

`NxPhysicsJointSupportTests` calls the pinned oracle at RVA `0x0009ad10` and the candidate on equivalent scratch objects. Baseline:

```text
joint_support island_teardown oracle=3/0/0001c392/2 candidate=3/0/0001c392/2 oracle_cleared=1 candidate_cleared=1 mismatches=0
joint_support coverage name=island_object_teardown calls=3 frees=2 cleared=1
```

The probe observes three callbacks in order 103, 101, 102, each with flag 1; both array frees; cleared pointers; and the untouched `+0x0c` canary. Mutation check: replacing the candidate method with an immediate return produced `oracle=3/0/0001c392/2 candidate=0/0/00000000/0 oracle_cleared=1 candidate_cleared=0 mismatches=4` and exit 1. Restoring the method returned the baseline above.

The coverage lines are registered as an oracle differential in both Phases 6 and 7. The retained proof manifest binds the fixture and oracle output; the exact teardown and coverage lines are also asserted by both phase gates.
