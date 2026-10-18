# CollisionObject owner-value accessor (`phys_fn_001201`)

IDA's body at RVA `0x00024870` acquires the scene read lock from `[self+0x14]`, follows the shape pointer at `[self+0x18]` to its owner, returns the owner's first dword, and releases the lock. `CollisionObject::nxOwnerValue()` in `Physics/src/ObjectModel.cpp` reproduces that path.

The `NxPhysicsShapeVtableTests` probe runs two distinct owner values through the oracle RVA and candidate method, and verifies the return value and released lock state. The initial red probe reports `mismatches=2`; after implementation, both candidate cases pass. Replacing the candidate's owner read with a null return is caught in both cases (`mismatches=2`). A clean rebuild removed the mutated object; the restored staged differential reports `collision owner accessor cases=2 mismatches=0` and the existing shape vtable digest remains exact (`cases=645 mismatches=0`).

Phase 3 advances to 118 closed / 274 deferred. The Phase 5 floor rises to 2,916 assertions. The fresh Win32 Release Phase 5 gate passes at 2,916/2,916 assertions, and its full transcript is compressed at `evidence/raw/phase3-collision-owner-accessor-001201-20261018/phase5-gate.log.gz`.
